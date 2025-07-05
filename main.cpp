#include "mbed.h"

BufferedSerial pc(MBED_UARTUSB); // connect to PC over MBED interface
BufferedSerial esp(MBED_UART2); // connect to ESP UART pins
DigitalInOut rst(p26, PIN_OUTPUT, OpenDrain, 1); // connect to ESP reset pin
DigitalInOut mode(p25, PIN_OUTPUT, OpenDrain, 1); // connect to ESP GPIO0

void reset_esp()
{
    rst = 0;
    wait_us(100 * 1000);
    rst = 1;
}

void flash_mode(bool enter)
{
    // To enter flash mode, set mode line low
    mode = (int)(!enter);
    reset_esp();
}

struct uart_passthrough
{
    FileHandle *in;
    FileHandle *out;
};

void passthrough(struct uart_passthrough *pt)
{
    char buf[1] = {0};
    ssize_t bytes = 0;

    while ((bytes = pt->in->read(buf, sizeof(buf))) >= 0)
    {
        if (pt->out->write(buf, bytes) < 0)
        {
            break;
        }
    }
}

int main()
{
    struct uart_passthrough pc2esp = {.in = &pc, .out = &esp};
    struct uart_passthrough esp2pc = {.in = &esp, .out = &pc};
    Thread t1, t2;

    // Place ESP8266 into flash mode
    flash_mode(true);

    t1.start(callback(passthrough, &pc2esp));
    t2.start(callback(passthrough, &esp2pc));

    t1.join();
    t2.join();

    // Exit flash mode
    flash_mode(false);

    return 0;
}
