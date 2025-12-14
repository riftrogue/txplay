"""Player controller - handles MPV playback via IPC."""

import subprocess
import socket
import json
import os
import time
import threading
import sys


class MPVPlayer:
    """Real MPV player controlled via IPC socket."""
    
    def __init__(self, socket_path=None):
        self.state = "stopped"  # stopped, playing, paused
        self.current = None
        # Use Termux-compatible socket path
        if socket_path is None:
            # Try $TMPDIR first (Termux), fallback to $HOME, then /tmp
            tmpdir = os.environ.get('TMPDIR') or os.path.expanduser('~')
            socket_path = os.path.join(tmpdir, 'txplay_mpv_socket')
        self.socket_path = socket_path
        self.process = None
        self.socket = None
        self._running = False
        self._monitor_thread = None
        self.on_track_end = None  # Callback when track ends
        self.position = 0  # Current playback position in seconds
        self.duration = 0  # Total track duration in seconds
        self.debug = True  # Enable debug logging
    
    def _log(self, msg):
        """Debug logging to stderr."""
        if self.debug:
            print(f"[MPV DEBUG] {msg}", file=sys.stderr, flush=True)
        
    def _start_mpv(self):
        """Start MPV process with IPC enabled."""
        if self.process and self.process.poll() is None:
            self._log("MPV already running")
            return  # Already running
        
        # Remove old socket if exists
        if os.path.exists(self.socket_path):
            try:
                os.remove(self.socket_path)
                self._log(f"Removed old socket: {self.socket_path}")
            except OSError as e:
                self._log(f"Failed to remove socket: {e}")
                pass
        
        # Start MPV with IPC socket - optimized for Termux
        self._log(f"Socket path: {self.socket_path}")
        self._log("Starting MPV process...")
        self.process = subprocess.Popen(
            [
                "mpv",
                f"--input-ipc-server={self.socket_path}",
                "--no-video",  # Audio only
                "--idle=yes",  # Keep MPV running
                "--no-terminal",  # Don't show MPV output
                "--audio-display=no",  # No album art
                "--really-quiet",  # Suppress messages
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self._log(f"MPV PID: {self.process.pid}")
        
        # Wait for socket to be created
        socket_created = False
        for i in range(30):  # 3 second timeout
            if os.path.exists(self.socket_path):
                self._log(f"Socket created after {i*0.1:.1f}s")
                time.sleep(0.1)  # Give socket time to be ready
                socket_created = True
                break
            
            # Check if process died
            if self.process.poll() is not None:
                stdout, stderr = self.process.communicate()
                self._log(f"MPV died! stdout: {stdout.decode()}")
                self._log(f"MPV stderr: {stderr.decode()}")
                raise RuntimeError("MPV process died before socket creation")
            
            time.sleep(0.1)
        
        if not socket_created:
            # Kill the process and raise error
            self.process.terminate()
            stdout, stderr = self.process.communicate(timeout=2)
            self._log(f"MPV timeout! stdout: {stdout.decode()}")
            self._log(f"MPV stderr: {stderr.decode()}")
            raise RuntimeError("MPV socket creation timeout")
        
        # Start monitoring thread
        if not self._running:
            self._running = True
            self._monitor_thread = threading.Thread(target=self._monitor_playback, daemon=True)
            self._monitor_thread.start()
            self._log("Monitoring thread started")
    
    def _send_command(self, command):
        """Send command to MPV via IPC socket."""
        try:
            if not os.path.exists(self.socket_path):
                self._log(f"Socket not found: {self.socket_path}")
                return None
            
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(2.0)  # Increased timeout
            sock.connect(self.socket_path)
            
            # Send command as JSON
            cmd_json = json.dumps(command) + "\n"
            self._log(f"Sending: {command}")
            sock.sendall(cmd_json.encode('utf-8'))
            
            # Read response
            response = b""
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
                if b"\n" in response:
                    break
            
            sock.close()
            
            if response:
                result = json.loads(response.decode('utf-8').strip())
                self._log(f"Response: {result}")
                return result
            return None
        except (socket.error, json.JSONDecodeError, OSError) as e:
            self._log(f"Command error: {e}")
            return None
    
    def _get_property(self, prop):
        """Get property value from MPV."""
        result = self._send_command({"command": ["get_property", prop]})
        if result and result.get("error") == "success":
            return result.get("data")
        return None
    
    def _monitor_playback(self):
        """Monitor playback status in background thread."""
        while self._running:
            try:
                # Check if track is still playing
                paused = self._get_property("pause")
                eof = self._get_property("eof-reached")
                
                if paused is not None:
                    self.state = "paused" if paused else "playing"
                
                # Update position and duration
                pos = self._get_property("time-pos")
                dur = self._get_property("duration")
                if pos is not None:
                    self.position = int(pos)
                if dur is not None:
                    self.duration = int(dur)
                
                # Check if track ended
                if eof and self.current and self.on_track_end:
                    self.current = None
                    self.state = "stopped"
                    self.on_track_end()  # Trigger callback
                
                time.sleep(0.5)  # Check every 500ms
            except:
                time.sleep(1)
    
    def play(self, target):
        """Start playing a file or URL."""
        try:
            self._log(f"Playing: {target}")
            self._start_mpv()
            
            # Load and play the file
            result = self._send_command({"command": ["loadfile", target]})
            
            # Check if load was successful
            if result and result.get("error") == "success":
                self._log(f"Successfully loaded: {target}")
                self.current = target
                self.state = "playing"
                self.position = 0
                self.duration = 0
            else:
                # Failed to load
                self._log(f"Failed to load: {result}")
                raise RuntimeError(f"Failed to load file: {target}")
        except Exception as e:
            # Reset state on error
            self._log(f"Play error: {e}")
            self.current = None
            self.state = "stopped"
            # Re-raise for debugging
            raise
    
    def pause(self):
        """Pause playback."""
        if self.state == "playing":
            self._send_command({"command": ["set_property", "pause", True]})
            self.state = "paused"
    
    def resume(self):
        """Resume playback."""
        if self.state == "paused":
            self._send_command({"command": ["set_property", "pause", False]})
            self.state = "playing"
    
    def stop(self):
        """Stop playback."""
        self._send_command({"command": ["stop"]})
        self.current = None
        self.state = "stopped"
        self.position = 0
        self.duration = 0
    
    def seek(self, seconds):
        """Seek forward or backward by seconds (can be negative)."""
        if self.current:
            self._send_command({"command": ["seek", seconds, "relative"]})
    
    def quit(self):
        """Quit MPV and cleanup."""
        self._running = False
        
        # Stop monitoring thread
        if self._monitor_thread and self._monitor_thread.is_alive():
            self._monitor_thread.join(timeout=1)
        
        # Try to quit MPV gracefully
        if self.process and self.process.poll() is None:
            try:
                self._send_command({"command": ["quit"]})
                # Give it a moment to quit
                time.sleep(0.2)
            except:
                pass
            
            # Force kill if still running
            try:
                if self.process.poll() is None:
                    self.process.terminate()
                    try:
                        self.process.wait(timeout=1)
                    except subprocess.TimeoutExpired:
                        self.process.kill()  # Force kill
                        self.process.wait(timeout=1)
            except:
                pass
            
            self.process = None
        
        # Clean up socket
        if os.path.exists(self.socket_path):
            try:
                os.remove(self.socket_path)
            except OSError:
                pass
