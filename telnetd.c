#include "telnetd.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "cli.h"
#include "libc.h"
static struct tcp_pcb *telnet_pcb = NULL;
struct tcp_pcb *active_client_pcb = NULL;

extern void puts(const char *s);
extern void puts_serial(const char *s);

static char telnet_buf[256];
static int telnet_idx = 0;

void telnet_puts(const char *s) {
    if (active_client_pcb) {
        // Send to TCP socket
        tcp_write(active_client_pcb, s, strlen(s), TCP_WRITE_FLAG_COPY);
        tcp_output(active_client_pcb);
    }
    // Also send to VGA/Serial for logging
    puts(s);
}

void telnet_set_color(uint8_t fg, uint8_t bg) {
    // 1. Call the original VGA set_color (fallback)
    extern void set_color(uint8_t fg, uint8_t bg);
    set_color(fg, bg);
    
    // 2. Send ANSI escape code to Telnet
    if (active_client_pcb) {
        // Very basic mapping from VGA to ANSI Foreground
        // VGA: 0=Black, 1=Blue, 2=Green, 3=Cyan, 4=Red, 5=Magenta, 6=Brown, 7=LightGray
        //      8=DarkGray, 9=LightBlue, 10=LightGreen, 11=LightCyan, 12=LightRed, 13=LightMagenta, 14=Yellow, 15=White
        // ANSI: 30=Black, 34=Blue, 32=Green, 36=Cyan, 31=Red, 35=Magenta, 33=Yellow, 37=White
        // Bright: 90=Black, 94=Blue, 92=Green, 96=Cyan, 91=Red, 95=Magenta, 93=Yellow, 97=White
        int ansi = 37; // default white
        if (fg == 0) ansi = 30;
        else if (fg == 1) ansi = 34;
        else if (fg == 2) ansi = 32;
        else if (fg == 3) ansi = 36;
        else if (fg == 4) ansi = 31;
        else if (fg == 5) ansi = 35;
        else if (fg == 6) ansi = 33;
        else if (fg == 7) ansi = 37;
        else if (fg == 8) ansi = 90;
        else if (fg == 9) ansi = 94;
        else if (fg == 10) ansi = 92;
        else if (fg == 11) ansi = 96;
        else if (fg == 12) ansi = 91;
        else if (fg == 13) ansi = 95;
        else if (fg == 14) ansi = 93;
        else if (fg == 15) ansi = 97;

        char buf[16];
        strcpy(buf, "\033[");
        
        // Custom simple itoa for ansi color
        int tens = ansi / 10;
        int ones = ansi % 10;
        int len = 2;
        buf[len++] = '0' + tens;
        buf[len++] = '0' + ones;
        buf[len++] = 'm';
        buf[len] = '\0';

        tcp_write(active_client_pcb, buf, len, TCP_WRITE_FLAG_COPY);
        tcp_output(active_client_pcb);
    }
}

static void print_telnet_prompt() {
    telnet_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    telnet_puts("\r\n>  ");
    telnet_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    telnet_puts("nkernel ");
    telnet_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    telnet_puts("X ");
    telnet_set_color(VGA_WHITE, VGA_BLACK);
}

static err_t telnet_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Connection closed
        active_client_pcb = NULL;
        tcp_close(tpcb);
        puts_serial("Telnet client disconnected.\n");
        return ERR_OK;
    }
    
    tcp_recved(tpcb, p->tot_len);
    
    char *data = (char *)p->payload;
    for (int i = 0; i < p->len; i++) {
        char c = data[i];
        
        #include "edit.h"
        if (in_editor_mode) {
            editor_handle_key(c);
            continue;
        }
        
        if (c == '\r' || c == '\n') {
            if (telnet_idx > 0) {
                telnet_buf[telnet_idx] = '\0';
                telnet_puts("\r\n");
                
                // Hook cli outputs to telnet
                extern void (*cli_puts)(const char *s);
                extern void (*cli_set_color)(uint8_t fg, uint8_t bg);
                void (*old_puts)(const char *) = cli_puts;
                void (*old_color)(uint8_t, uint8_t) = cli_set_color;
                
                cli_puts = telnet_puts;
                cli_set_color = telnet_set_color;
                
                cli_execute(telnet_buf);
                
                // Restore hooks
                cli_puts = old_puts;
                cli_set_color = old_color;
            }
            telnet_idx = 0;
            if (!in_editor_mode) {
                print_telnet_prompt();
            }
        } else if (c == '\b' || c == 127) { // Backspace
            if (telnet_idx > 0) {
                telnet_idx--;
                telnet_puts("\b \b");
            }
        } else if (c == '\t') {
            // Hook cli_puts to telnet_puts for autocomplete
            extern void (*cli_puts)(const char *s);
            void (*old_puts)(const char *) = cli_puts;
            cli_puts = telnet_puts;
            
            cli_autocomplete(telnet_buf, &telnet_idx, 256);
            
            cli_puts = old_puts;
        } else {
            if (telnet_idx < 255) {
                telnet_buf[telnet_idx++] = c;
                // Echo
                char str[2] = {c, '\0'};
                telnet_puts(str);
            }
        }
    }
    
    pbuf_free(p);
    return ERR_OK;
}

static err_t telnet_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    puts_serial("Telnet client connected!\n");
    if (active_client_pcb != NULL) {
        // Reject if we already have a client (simple single-client server)
        tcp_close(newpcb);
        return ERR_OK;
    }
    
    active_client_pcb = newpcb;
    tcp_recv(newpcb, telnet_recv);
    
    telnet_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    telnet_puts("Welcome to NKernel Router!\r\n");
    print_telnet_prompt();
    
    return ERR_OK;
}

void telnetd_init(void) {
    telnet_pcb = tcp_new();
    if (telnet_pcb != NULL) {
        err_t err = tcp_bind(telnet_pcb, IP_ADDR_ANY, 23);
        if (err == ERR_OK) {
            telnet_pcb = tcp_listen(telnet_pcb);
            tcp_accept(telnet_pcb, telnet_accept);
            puts_serial("Telnet Server initialized on Port 23.\n");
        } else {
            puts_serial("Telnet bind failed.\n");
        }
    }
}
