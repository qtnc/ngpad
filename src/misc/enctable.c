#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

struct Encoding {
int cp;
char name[16];
char desc[64];
} encodings[] = {
{ 737, "737", "Greek DOS" }, 
{ 775, "775", "Baltic DOS"  }, 
{ 857, "857", "Turkish DOS" }, 
{ 858, "858", "Latin-9 DOS with euro" }, 
{ 860, "860", "Portuguese DOS" },
{ 861, "861", "Islandic DOS" },
{ 862, "862", "Hebrew DOS" },
{ 863, "863", "French canadian DOS" },
{ 864, "864", "Arabic DOS" },
{ 865, "865", "Nordic DOS" },
{ 869, "869", "Modern greek DOS" },
{ 500, "500\0ebcdic\0", "IBM EBCDIC International" },
{ 37, "037", "IBM EBCDIC Canada" },
{ 870, "870", "IBM EBCDIC Multilingual latin-2" },
{ 875, "875", "IBM EBCDIC Modern greek" },
{ 1047, "01047", "IBM EBCDIC Western european latin-1 / Open system" },
{ 1026, "1026", "IBM EBCDIC Turkish latin-5" },
{ 1140, "01140", "IBM EBCDIC Canada + euro" },
{ 1141, "01141", "IBM EBCDIC Germany + euro" },
{ 1142, "01142", "IBM EBCDIC Denmark Norway + euro" },
{ 1143, "01143", "IBM EBCDIC Finland Sweden + euro" },
{ 1144, "01144", "IBM EBCDIC Italy + euro" },
{ 1145, "01145", "IBM EBCDIC Latin America Spain + euro" },
{ 1146, "01146", "IBM EBCDIC United Kingdom + euro" },
{ 1147, "01147", "IBM EBCDIC France + euro" },
{ 1148, "01148", "IBM EBCDIC International + euro" },
{ 20924, "00924", "IBM EBCDIC Western europen latin-1 + euro / Open system" },
{ 1149, "01149", "IBM EBCDIC Islandic + euro" },
{ 20273, "273", "IBM EBCDIC Germany" },
{ 20277, "277", "IBM EBCDIC Denmark Norway" },
{ 20278, "278", "IBM EBCDIC Finland Sweden" },
{ 20280, "280", "IBM EBCDIC Italy" },
{ 20284, "284", "IBM EBCDIC Latin America Spain" },
{ 20285, "285", "IBM EBCDIC United Kingdom" },
{ 20290, "290", "IBM EBCDIC Japanese Katakana extended" },
{ 20297, "297", "IBM EBCDIC France" },
{ 20420, "420", "IBM EBCDIC Arabic" },
{ 20423, "423", "IBM EBCDIC Greek" },
{ 20905, "905", "IBM EBCDIC Turkish" },
{ 20424, "424", "IBM EBCDIC Hebrew" },
{ 20833, "833", "IBM EBCDIC Korean extended" },
{ 20838, "838", "IBM EBCDIC Thai" },
{ 20871, "871", "IBM EBCDIC Islandic" },
{ 20880, "880", "IBM EBCDIC Cyrillic Russian" },
{ 21025, "1025", "IBM EBCDIC Cyrillic Serbian Bulgarian" },
{ 29001, "europa3", "Europa 3" },
{ 20105, "ia5", "IRV international alphabet no. 5 Western european" },
{ 20106, "ia5-de", "IA5 German" },
{ 20107, "ia5-se", "IA5 Swedish" },
{ 20108, "ia5-no", "IA5 Norwegian" },
{ 20261, "t.61", "T.61" },
{ 20269, "20269", "ISO-6937 Non spacing accent" },
{ 708, "asmo-708", "Arabic ASMO 708" },
{ 709, "asmo-449", "Arabic ASMO 449+ BCON v4" },
{ 710, "asmo-710", "Arabic transparent" },
{ 720, "720", "Arabic transparent ASMO / Arabic DOS" },
{ 0, NULL, NULL }
};

int main (int argc, char** argv) {
FILE* fp = fopen("enctable.txt", "wb");
char cbuf[257] = {0};
wchar_t wbuf[257] = { 0 };
for (int i=0; i<256; i++) cbuf[i] = i;
for (int i=0; encodings[i].cp; i++) {
struct Encoding* enc = &encodings[i];
int re = MultiByteToWideChar(enc->cp, MB_PRECOMPOSED, cbuf, 256, wbuf, 256);
if (re<=0) {
printf("Failed! %d, %s, %s => %d, %d\n", enc->cp, enc->name, enc->desc, re, GetLastError());
continue;
}
fwrite(&enc->cp, 4, 1, fp);
fwrite(enc->name, 16, 1, fp);
fwrite(enc->desc, 64, 1, fp);
fwrite(wbuf, 512, 1, fp);
printf("%d, %s, %s\n", enc->cp, enc->name, enc->desc);
}
fclose(fp);
return 0;
}
