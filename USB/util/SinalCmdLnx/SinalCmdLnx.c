// Exemplo simples de controle do sinalizador ligado a USB
// Daniel Quadros - 19/09/16 - https://dqsoft.blogpsot.com
//
// Colocar na linha de comando os LEDs que devem ficar acesos
// R (red = vermelho), O (orange = laranja), G (green = verde), 
// W (white = branco)
//
// Ex: "SinalCmdLnx RG" acende os LEDS vermelho e verde e 
//     apaga os demais
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hiddev.h>
#include <linux/input.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#define FALSE   0
#define TRUE    1

// Identificacao do nosso dispositivo
#define MEU_VID	0x16C0
#define MEU_PID	0x05DF

// Rotinas locais
int BuscaDisp  (void);
void Info      (int fd);
int SetFeature (int fd, int leds);

// Programa principal
int main (int argc, char *argv[]) {
    int fd;
    int leds;
    
    // Procura o nosso dispositivo
    fd = BuscaDisp();
    if (fd < 0) {
        return 1;
    }
    Info(fd);

    // Programa o novo valor dos LEDs
    leds = 0;
    if (argc > 1) {
        char *p = argv[1];
        while (*p) {
            switch (*p) {
                case 'r': case 'R':	leds |= 0x08; break;
                case 'o': case 'O':	leds |= 0x04; break;
                case 'g': case 'G':	leds |= 0x20; break;
                case 'w': case 'W':	leds |= 0x10; break;
            }
            p++;
        }
    }
    if (!SetFeature (fd, leds)) {
        return 2;
    }
    
    return 0;
}

// Procura o nosso dispositivo
// Retorna handle se tiver sucesso, -1 se ocorrer erro
int BuscaDisp() {
    struct hiddev_devinfo device_info;
    char path[256];
    int fd;
    int i;
        
    for(i = 0; i < 32; i++) {
        sprintf(path,"/dev/usb/hiddev%d", i);
        fd = open (path, O_RDONLY);
        if (fd >= 0){
            ioctl(fd, HIDIOCGDEVINFO, &device_info);
            if ((device_info.vendor == MEU_VID) &&
                (device_info.product == MEU_PID)) {
                printf ("Dispositivo achado em %s\n", path);
                return fd;
            }
            close (fd);
        }
    }
    printf ("Nao achou dispositivo\n");
    return -1;
}

// Mostra informacoes do dispositivo
void Info (int fd) {
    struct hiddev_string_descriptor Hstring1, Hstring2;

    Hstring1.index = 1;
    Hstring1.value[0] = 0;
    Hstring2.index = 2;
    Hstring2.value[0] = 0;
    ioctl(fd, HIDIOCGSTRING, &Hstring1);
    ioctl(fd, HIDIOCGSTRING, &Hstring2);
    printf("Fabricante: %s\nProduto: %s\n", Hstring1.value,Hstring2.value);
}

// Programa os LEDs
// Retorna TRUE se tiver sucesso, FALSE se ocorrer erro
int SetFeature (int fd, int leds) {
    struct hiddev_report_info rep_info_u;
    struct hiddev_usage_ref_multi ref_multi_u;
    
    rep_info_u.report_type = HID_REPORT_TYPE_FEATURE;
    rep_info_u.report_id   = HID_REPORT_ID_FIRST;
    rep_info_u.num_fields  = 1;
    
    ref_multi_u.uref.report_type = HID_REPORT_TYPE_FEATURE;
    ref_multi_u.uref.report_id   = HID_REPORT_ID_FIRST;
    ref_multi_u.uref.field_index = 0;
    ref_multi_u.uref.usage_index = 0;
    ref_multi_u.num_values =  3;
    ref_multi_u.values[0] = 1;  // identificacao do dispositivo
    ref_multi_u.values[1] = 0;		// 
    ref_multi_u.values[2] = leds;	// programacao dos leds

    ioctl(fd,HIDIOCSUSAGES, &ref_multi_u);
    ioctl(fd,HIDIOCSREPORT, &rep_info_u);
    
    return TRUE;
}
