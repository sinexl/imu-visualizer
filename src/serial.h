#ifndef SERIAL_H_
#define SERIAL_H_

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>

int open_serial(const char* path);

#ifdef CSERIAL_IMPLEMENTATION


//partially taken from https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
int open_serial(const char* path) {
    int fd = open(path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Error: could not open %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error: could not retrieve serial port info: %s\n", strerror(errno));
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
        fprintf(stderr, "Error: could not update tty: %s\n", strerror(errno));
        return -1;
    }
    return fd;
}

#endif //  CSERIAL_IMPLEMENTATION

#endif // SERIAL_H_
