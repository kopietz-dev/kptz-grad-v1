#include "pico/stdlib.h"
#include "tusb.h"
#include <cstdint>
#include <cstdio>

template <typename T, uint16_t N> class StaticVector {
public:
  void push_back(const T &v) {
    if (m_Size < N) {
      m_Buffer[m_Size] = v;
      m_Size++;
    }
  }
  T pop_back() {
    if (m_Size > 0) {
      m_Size--;
      return m_Buffer[m_Size];
    }
    return T{};
  }
  uint16_t size() const { return m_Size; }

private:
  uint16_t m_Size = 0;
  T m_Buffer[N];
};
