#ifndef SLICE_H
#define SLICE_H

template <typename T>
class Slice {
   T* _data;
   size_t _num_elements;

  public:
   Slice(T* data, size_t num_elements) : _data(data), _num_elements(num_elements) {}

   const T* data() const noexcept { return _data; }
   T* data() noexcept { return _data; }
   size_t size() const noexcept { return _num_elements; }
};

#endif
