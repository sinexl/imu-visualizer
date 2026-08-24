#ifndef SERIAL_H_
#define SERIAL_H_

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
#define SERIALDEF extern "C"
#else
#define SERIALDEF 
#endif

typedef struct {
    int fd; 
} SerialPort; 

typedef int SerialError; 

#define SERIAL_SUCCESS 0
// TODO: Make this thread_local
static SerialError serial__last_error = SERIAL_SUCCESS;  

#define SERIAL__RETURN_ERROR(value) do {   \
    serial__last_error = errno;            \
    return (value);                        \
} while (0)

#define SERIAL__RETURN_SUCCESS(value) do { \
    serial__last_error = SERIAL_SUCCESS;   \
    return (value);                        \
} while(0)


// Opens Serial port in non-blocking mode. See serial_read().
// Return value: 
// On error, false.
SERIALDEF bool serial_open(const char* path, SerialPort* out);
// Return value:
// On success, amount of bytes read.
// On timeout, 0 is returned
// On errror, -1 is returned.
SERIALDEF int serial_read(SerialPort port, void* out, size_t out_size_bytes);
// Return value:
// On error, false.
SERIALDEF bool serial_close(SerialPort port); 
SERIALDEF SerialError serial_get_last_error(); 
// Note: the returned pointer will be invalidated on a each subsequent call to this function.
SERIALDEF const char* serial_stringify_error(SerialError error);

#ifdef CSERIAL_IMPLEMENTATION


//source: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
SERIALDEF bool serial_open(const char* path, SerialPort* out) {
    // TODO: Notify user when NULL is passed
    if (out == NULL || path == NULL) return false;
    
    int fd = open(path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        SERIAL__RETURN_ERROR(false);
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        SERIAL__RETURN_ERROR(false);
    }
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        SERIAL__RETURN_ERROR(false);
    }

    out->fd = fd;
    SERIAL__RETURN_SUCCESS(true);
}

SERIALDEF int serial_read(SerialPort port, void* out, size_t out_size_bytes) {
    int n = read(port.fd, out, out_size_bytes);
    if (n > 0) SERIAL__RETURN_SUCCESS(n);
    if (n < 0 && (errno = EAGAIN || errno == EWOULDBLOCK)) SERIAL__RETURN_SUCCESS(0);

    SERIAL__RETURN_ERROR(-1);
}

SERIALDEF bool serial_close(SerialPort port) { 
    if (close(port.fd) == -1)  SERIAL__RETURN_ERROR(false);
    SERIAL__RETURN_SUCCESS(true);
}

SERIALDEF SerialError serial_get_last_error() {
    return serial__last_error; 
}

SERIALDEF const char* serial_stringify_error(SerialError error) {
    return strerror(error);
}

#endif //  CSERIAL_IMPLEMENTATION

#endif // SERIAL_H_
