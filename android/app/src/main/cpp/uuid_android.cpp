// Android platform adapter; upstream LAF is MIT licensed.
#include "base/uuid.h"
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
namespace base {
Uuid Uuid::Generate() {
  Uuid out;
  const int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  if (fd < 0) throw std::runtime_error("Cannot open Android random source");
  size_t done = 0;
  while (done < 16) {
    ssize_t n = read(fd, out.bytes() + done, 16 - done);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) { close(fd); throw std::runtime_error("Cannot generate UUID"); }
    done += size_t(n);
  }
  close(fd);
  out.bytes()[6] = (out.bytes()[6] & 0x0f) | 0x40;
  out.bytes()[8] = (out.bytes()[8] & 0x3f) | 0x80;
  return out;
}
}
