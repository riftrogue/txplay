"""Player controller - handles MPV playback via IPC."""

import subprocess
import socket
import json
import os
import time
import threading


class MPVPlayer:
    """Real MPV player controlled via IPC socket."""
    
    def __init__(self, socket_path="/tmp/txplay_mpv_socket"):
        self.state = "stopped"  # stopped, playing, paused
        self.current = None
        self.socket_path = socket_path
        self.process = None
        self.socket = None
        self._running = False
        self._monitor_thread = None
        self.on_track_end = None  # Callback when track ends
        self.position = 0  # Current playback position in seconds
        self.duration = 0  # Total track duration in seconds
        
    def _start_mpv(self):
        """Start MPV process with IPC enabled."""
        if self.process and self.process.poll() is None:
            return  # Already running
        
        # Remove old socket if exists
        if os.path.exists(self.socket_path):
            os.remove(self.socket_path)
        
        # Start MPV with IPC socket
        self.process = subprocess.Popen(
            [
                "mpv",
                f"--input-ipc-server={self.socket_path}",
                "--no-video",  # Audio only
                "--idle=yes",  # Keep MPV running
                "--no-terminal",  # Don't show MPV output
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        
        # Wait for socket to be created
        for _ in range(50):  # 5 second timeout
            if os.path.exists(self.socket_path):
                time.sleep(0.1)  # Give it a moment to be ready
                break
            time.sleep(0.1)
        
        # Start monitoring thread
        if not self._running:
            self._running = True
            self._monitor_thread = threading.Thread(target=self._monitor_playback, daemon=True)
            self._monitor_thread.start()
    
    def _send_command(self, command):
        """Send command to MPV via IPC socket."""
        try:
            if not os.path.exists(self.socket_path):
                return None
            
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(1.0)
            sock.connect(self.socket_path)
            
            # Send command as JSON
            cmd_json = json.dumps(command) + "\n"
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
                return json.loads(response.decode('utf-8').strip())
            return None
        except (socket.error, json.JSONDecodeError, OSError):
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
        self._start_mpv()
        
        # Load and play the file
        self._send_command({"command": ["loadfile", target]})
        self.current = target
        self.state = "playing"
        self.position = 0
        self.duration = 0
    
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
        if self._monitor_thread:
            self._monitor_thread.join(timeout=2)
        
        if self.process:
            self._send_command({"command": ["quit"]})
            self.process.wait(timeout=2)
            self.process = None
        
        if os.path.exists(self.socket_path):
            os.remove(self.socket_path)
