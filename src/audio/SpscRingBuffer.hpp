#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <algorithm>

namespace txplay::audio {

template <typename T>
class SpscRingBuffer {
public:
    explicit SpscRingBuffer(size_t capacity) 
        : buffer_(capacity + 1), capacity_(capacity + 1) {
        read_index_.store(0, std::memory_order_relaxed);
        write_index_.store(0, std::memory_order_relaxed);
    }

    size_t write_available() const {
        size_t r = read_index_.load(std::memory_order_acquire);
        size_t w = write_index_.load(std::memory_order_relaxed);
        if (w >= r) {
            return capacity_ - 1 - (w - r);
        } else {
            return r - w - 1;
        }
    }

    size_t read_available() const {
        size_t r = read_index_.load(std::memory_order_relaxed);
        size_t w = write_index_.load(std::memory_order_acquire);
        if (w >= r) {
            return w - r;
        } else {
            return capacity_ - (r - w);
        }
    }

    size_t write(const T* data, size_t count) {
        size_t r = read_index_.load(std::memory_order_acquire);
        size_t w = write_index_.load(std::memory_order_relaxed);
        
        size_t available = (w >= r) ? (capacity_ - 1 - (w - r)) : (r - w - 1);
        size_t to_write = std::min(count, available);
        if (to_write == 0) return 0;

        size_t first_part = std::min(to_write, capacity_ - w);
        std::copy_n(data, first_part, buffer_.data() + w);
        
        if (first_part < to_write) {
            std::copy_n(data + first_part, to_write - first_part, buffer_.data());
        }

        size_t next_w = (w + to_write) % capacity_;
        write_index_.store(next_w, std::memory_order_release);
        
        return to_write;
    }

    size_t read(T* data, size_t count) {
        size_t r = read_index_.load(std::memory_order_relaxed);
        size_t w = write_index_.load(std::memory_order_acquire);
        
        size_t available = (w >= r) ? (w - r) : (capacity_ - (r - w));
        size_t to_read = std::min(count, available);
        if (to_read == 0) return 0;

        size_t first_part = std::min(to_read, capacity_ - r);
        std::copy_n(buffer_.data() + r, first_part, data);
        
        if (first_part < to_read) {
            std::copy_n(buffer_.data(), to_read - first_part, data + first_part);
        }

        size_t next_r = (r + to_read) % capacity_;
        read_index_.store(next_r, std::memory_order_release);
        
        return to_read;
    }

    // Force flush via epoch sync ONLY (consumer calls this safely)
    void flush_consumer_only() {
        size_t w = write_index_.load(std::memory_order_acquire);
        read_index_.store(w, std::memory_order_release);
    }
    
private:
    std::vector<T> buffer_;
    size_t capacity_;
    alignas(64) std::atomic<size_t> read_index_;
    alignas(64) std::atomic<size_t> write_index_;
};

} // namespace txplay::audio
