#pragma once

// Operator for placement new
template <typename T>
constexpr void* operator new(size_t size, T* ptr) {
  return ptr;
}
