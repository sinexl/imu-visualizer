#ifndef SERIAL_H_
#define SERIAL_H_

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
    #define SERIALDEF extern "C"
    #define SERIAL_LIT(type) type
#else
    #define SERIALDEF
    #define SERIAL_LIT(type) (type)
#endif // __cplusplus

typedef struct {
#ifdef __linux__
    int fd; 
#elif _WIN32
    #error "Not implemented yet for windows"
#endif
} SerialPort; 

#ifdef __linux__ 
    typedef int SerialError; 
    #define SERIAL_SUCCESS 0
    
    typedef enum {
        SERIALB_9600    = B9600, 
        SERIALB_19200   = B19200, 
        SERIALB_38400   = B38400, 
        SERIALB_57600   = B57600,
        SERIALB_115200  = B115200,
        SERIALB_230400  = B230400
    } SerialBaudRate; 
    
    typedef enum {
        SERIALDB_5 = CS5,
        SERIALDB_6 = CS6,
        SERIALDB_7 = CS7,
        SERIALDB_8 = CS8
    } SerialDataBits;
#elif _WIN32
    #error "Not implemented yet for windows"
#endif // __linux__

typedef enum {
    SERIALSB_1,
    SERIALSB_2, 
} SerialStopBits; 

typedef enum {
    SERIALP_NONE,
    SERIALP_ODD,
    SERIALP_EVEN,
} SerialParity;

typedef struct {
    SerialBaudRate baud_rate;
    SerialDataBits data_bits;
    SerialParity   parity;
    SerialStopBits stop_bits;
} SerialConfiguration;


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

// default configuration: 115200 8N1
SERIALDEF SerialConfiguration serial_cfg_default();

// Opens Serial port in non-blocking mode. See serial_read().
// Parameters:
// Path: null-terminated string with path to device file (i. e /dev/ttyACM0 on Linux)
// Cfg:  Pointer to SerialConfiguration structure. If NULL is provided, serial_cfg_default() will be used.
// Out:  valid pointer to zero-initialized SerialPort structure (output parameter)
// Return value: 
// On error, false.
SERIALDEF bool serial_open(const char* path, const SerialConfiguration* cfg, SerialPort* out);
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

#ifdef SERIAL_IMPLEMENTATION


//source: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
SERIALDEF bool serial_open(const char* path, const SerialConfiguration* cfg, SerialPort* out) {
    // TODO: Notify user when NULL is passed
    if (out == NULL || path == NULL) return false;
    
    SerialConfiguration configuration = serial_cfg_default();
    if (cfg != NULL) configuration = *cfg;

    int fd = open(path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        SERIAL__RETURN_ERROR(false);
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        SERIAL__RETURN_ERROR(false);
    }
    cfsetospeed(&tty, configuration.baud_rate);
    cfsetispeed(&tty, configuration.baud_rate); // Baud rate

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= configuration.data_bits;    // data bits.

    switch(configuration.parity) {
        case SERIALP_NONE: {
            tty.c_cflag &= ~PARENB;
            break; 
        }  
        case SERIALP_EVEN: {
            tty.c_cflag |=  PARENB;
            tty.c_cflag &= ~PARODD; // even (unset odd)
            break;
        }
        case SERIALP_ODD: {
            tty.c_cflag |=  PARENB | PARODD;
            break; 
        } 
    }
    switch(configuration.stop_bits) {
        case SERIALSB_1: tty.c_cflag &= ~CSTOPB; break;
        case SERIALSB_2: tty.c_cflag |=  CSTOPB; break; 
    }

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

SERIALDEF SerialConfiguration serial_cfg_default () {
    return 
        (SERIAL_LIT(SerialConfiguration) { SERIALB_115200, SERIALDB_8, SERIALP_NONE, SERIALSB_1 }); 
}

#endif //  SERIAL_IMPLEMENTATION

#endif // SERIAL_H_
