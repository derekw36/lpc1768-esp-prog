#include "mbed.h"

#define ESP_BAUD 9600

BufferedSerial pc(MBED_UARTUSB); // connect to PC over MBED interface
BufferedSerial esp(MBED_UART2); // connect to ESP UART pins
DigitalOut rst(p26, 1); // connect to ESP reset pin
DigitalOut mode(p25, 1); // connect to ESP GPIO0

Thread t1;
Thread t2;

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
} pc2esp, esp2pc;

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
    // ESP8266 default baud rate is 9600
    pc.set_baud(ESP_BAUD);
    esp.set_baud(ESP_BAUD);

    pc2esp.in = &pc;
    pc2esp.out = &esp;
    esp2pc.in = &esp;
    esp2pc.out = &pc;

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
