#include <pc.h>
#include <dos.h>
#include <conio.h>
#include <sys/nearptr.h>
#include <stdio.h>
#include <string.h>

#define VRAM 0xA0000
unsigned char *video_mem, back_buffer[64000], levelGrid[1000][8];
int viewPos = 0, curX = 0, curY = 5, beep_t = 0;

void game_beep(int f) {
    if(f==0){ outp(0x61, inp(0x61)&0xFC); return; }
    int d=1193180/f; outp(0x43, 0xB6); outp(0x42, d&0xFF); outp(0x42, d>>8); outp(0x61, inp(0x61)|3);
}

void drawRect(int x, int y, int w, int h, int c) {
    for(int i=0; i<w; i++) for(int j=0; j<h; j++) {
        int cx=x+i, cy=y+j; if(cx>=0 && cx<320 && cy>=0 && cy<200) back_buffer[cy*320+cx]=c;
    }
}

void drawBigChar(int x, int y, char ch, int col) {
    if(ch=='S') { drawRect(x,y,10,2,col); drawRect(x,y,2,6,col); drawRect(x,y+5,10,2,col); drawRect(x+8,y+5,2,6,col); drawRect(x,y+10,10,2,col); }
    else if(ch=='A') { drawRect(x,y,10,2,col); drawRect(x,y,2,12,col); drawRect(x+8,y,2,12,col); drawRect(x,y+6,10,2,col); }
    else if(ch=='V') { drawRect(x,y,2,8,col); drawRect(x+8,y,2,8,col); drawRect(x+2,y+8,2,2,col); drawRect(x+6,y+8,2,2,col); drawRect(x+4,y+10,2,2,col); }
    else if(ch=='E') { drawRect(x,y,10,2,col); drawRect(x,y,2,12,col); drawRect(x,y+5,8,2,col); drawRect(x,y+10,10,2,col); }
    else if(ch=='T') { drawRect(x,y,10,2,col); drawRect(x+4,y,2,12,col); }
    else if(ch=='O') { drawRect(x,y,10,2,col); drawRect(x,y+10,10,2,col); drawRect(x,y,2,12,col); drawRect(x+8,y,2,12,col); }
    else if(ch=='L') { drawRect(x,y,2,12,col); drawRect(x,y+10,10,2,col); }
    else if(ch=='D') { drawRect(x,y,8,2,col); drawRect(x,y,2,12,col); drawRect(x,y+10,8,2,col); drawRect(x+8,y+2,2,8,col); }
    else if(ch=='.') { drawRect(x+4,y+10,2,2,col); }
    else if(ch=='?') { drawRect(x,y,8,2,col); drawRect(x+8,y+2,2,4,col); drawRect(x+4,y+6,4,2,col); drawRect(x+4,y+10,2,2,col); }
    else if(ch=='Y') { drawRect(x,y,2,5,col); drawRect(x+8,y,2,5,col); drawRect(x,y+5,10,2,col); drawRect(x+4,y+7,2,5,col); }
    else if(ch=='N') { drawRect(x,y,2,12,col); drawRect(x+8,y,2,12,col); for(int i=0;i<10;i++) drawRect(x+i,y+i,2,2,col); }
    else if(ch=='/') { for(int i=0; i<10; i++) drawRect(x+9-i, y+i, 2, 2, col); }
}

void drawBigText(int x, int y, char *s, int col) {
    while(*s) { if(*s!=' ') drawBigChar(x, y, *s, col); x += 12; s++; }
}

void drawOrb(int x, int y, int c) {
    int r=7; for(int i=-r; i<=r; i++) for(int j=-r; j<=r; j++) if(i*i+j*j<=r*r) {
        int px=x+i, py=y+j; if(px>=0 && px<320 && py>=0 && py<200) back_buffer[py*320+px]=(i*i+j*j>(r-2)*(r-2))?0:c;
    }
}

void renderGrid() {
    memset(back_buffer, 15, 64000);
    for(int i=0; i<16; i++) for(int j=0; j<8; j++) {
        int x=i*20, y=j*20+20; unsigned char t=levelGrid[viewPos+i][j];
        for(int s=0; s<20; s++) { back_buffer[y*320+x+s]=7; if(y+s<200) back_buffer[(y+s)*320+x]=7; }
        if(t==4) drawRect(x+1,y+1,18,18,0);
        else if(t==1) { for(int k=0; k<10; k++) drawRect(x+5+k, y+15-k, 1, k, 0); }
        else if(t==11) { for(int k=0; k<10; k++) drawRect(x+5+k, y+5, 1, k, 0); }
        else if(t>=2 && t<=6 || t==9 || t==12) {
            int cols[]={14,13,11,10,4,0};
            int ci = (t==9)?4 : (t==12)?5 : (t==5)?2 : (t==6)?3 : t-2;
            drawOrb(x+10, y+10, cols[ci]);
        }
    }
    drawRect(curX*20+2, curY*20+38, 16, 2, 1);
}

int main() {
    __asm__ ("int $0x10"::"a"(0x13)); __djgpp_nearptr_enable();
    video_mem = (unsigned char *)(__djgpp_conventional_base + VRAM);
    memset(levelGrid, 0, 8000);
    FILE *fin=fopen("LEVEL.DAT","rb"); if(fin){ fgetc(fin); fread(levelGrid,1,8000,fin); fclose(fin); }
    while(1) {
        if(beep_t > 0) { beep_t--; if(beep_t == 0) game_beep(0); }
        renderGrid();
        if(kbhit()) {
            int k=getch(); if(k==27) break;
            if(k=='s'||k=='S') {
                game_beep(400);
                for(int i=1; i<=10; i++) {
                    int w=25*i, h=8*i; int x=160-w/2, y=100-h/2;
                    renderGrid(); drawRect(x+4, y+4, w, h, 0); drawRect(x, y, w, h, 1);
                    if(i==10) { drawBigText(52, 85, "SAVE TO LEVEL.DAT?", 15); drawBigText(130, 110, "Y / N", 15); }
                    while(inp(0x3DA)&8); while(!(inp(0x3DA)&8)); memcpy(video_mem, back_buffer, 64000); delay(15);
                }
                while(1) { if(kbhit()) { int c=getch(); 
                    if(c=='y'||c=='Y'){ 
                        FILE *f=fopen("LEVEL.DAT","wb"); fputc(6,f); fwrite(levelGrid,1,8000,f); fclose(f); 
                        game_beep(800); beep_t=10; break; 
                    }
                    if(c=='n'||c=='N'||c==27) break;
                } } continue;
            }
            if(k=='1') { levelGrid[viewPos+curX][curY]=1; game_beep(1000); beep_t=1; }
            if(k=='r'||k=='R') {
                game_beep(1100); beep_t=1;
                if(levelGrid[viewPos+curX][curY]==1) levelGrid[viewPos+curX][curY]=11;
                else if(levelGrid[viewPos+curX][curY]==11) levelGrid[viewPos+curX][curY]=1;
            }
            if(k=='2') {
                game_beep(1200); beep_t=1;
                int t=levelGrid[viewPos+curX][curY], ty[]={2,3,5,6,9,12}, i=-1;
                for(int j=0; j<6; j++) if(ty[j]==t) i=j;
                levelGrid[viewPos+curX][curY]=ty[(i+1)%6];
            }
            if(k=='4'||k==' ') { levelGrid[viewPos+curX][curY]=4; game_beep(900); beep_t=1; }
            if(k==8) { levelGrid[viewPos+curX][curY]=0; game_beep(200); beep_t=2; }
            if(k==0) { k=getch();
                game_beep(600); beep_t=1;
                if(k==77){ if(curX<15) curX++; else if(viewPos<984) viewPos++; }
                if(k==75){ if(curX>0) curX--; else if(viewPos>0) viewPos--; }
                if(k==72 && curY>0) curY--; if(k==80 && curY<7) curY++;
            }
        }
        memcpy(video_mem, back_buffer, 64000);
    }
    game_beep(0); __asm__ ("int $0x10"::"a"(3)); return 0;
}
