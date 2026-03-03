#include <pc.h>
#include <dos.h>
#include <conio.h>
#include <sys/nearptr.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define VRAM 0xA0000
unsigned char *video_mem, back_buffer[64000], levelGrid[1000][8];
int gameState=0, oi=0, ox=0, lastOrb=-1, beep_t=0, maxIdx=0, canActivateOrb=1;
float py=155.0, vy=0.0, gravityDir=1.0, angle=0.0;

void game_beep(int f) {
    if(f==0){ outp(0x61, inp(0x61)&0xFC); return; }
    int d=1193180/f; outp(0x43, 0xB6); outp(0x42, d&0xFF); outp(0x42, d>>8); outp(0x61, inp(0x61)|3);
}

void drawRect(int x, int y, int w, int h, int c) {
    for(int i=0; i<w; i++) for(int j=0; j<h; j++) {
        int cx=x+i, cy=y+j; if(cx>=0 && cx<320 && cy>=0 && cy<200) back_buffer[cy*320+cx]=c;
    }
}

void drawOrb(int x, int y, int c) {
    int r=7; for(int i=-r; i<=r; i++) for(int j=-r; j<=r; j++) {
        if(i*i+j*j<=r*r) { int px=x+i, py=y+j; if(px>=0 && px<320 && py>=0 && py<200) back_buffer[py*320+px]=(i*i+j*j>(r-2)*(r-2))?0:c; }
    }
}

void drawSpike(int x, int y, int inv) {
    if(!inv) { for(int i=0; i<10; i++) drawRect(x+5+i, y+15-i, 1, i, 0); }
    else { for(int i=0; i<10; i++) drawRect(x+5+i, y+5, 1, i, 0); }
}

void drawRotatedCube(int x, int y, float a) {
    float s=sin(a*3.14/180.0), c=cos(a*3.14/180.0);
    for(int i=-9; i<9; i++) for(int j=-9; j<9; j++) {
        int nx=(int)(i*c-j*s)+x+9, ny=(int)(i*s+j*c)+y+9;
        if(nx>=0 && nx<320 && ny>=0 && ny<200) back_buffer[ny*320+nx]=0;
    }
}

void reset_player() { gameState=0; oi=0; ox=0; py=155; vy=0; gravityDir=1.0; angle=0; lastOrb=-1; canActivateOrb=1; game_beep(200); beep_t=2; }

int main() {
    __asm__ ("int $0x10"::"a"(0x13)); __djgpp_nearptr_enable();
    video_mem = (unsigned char *)(__djgpp_conventional_base + VRAM);
    memset(levelGrid, 0, 8000);
    FILE *f = fopen("LEVEL.DAT","rb"); if(f){ fgetc(f); fread(levelGrid,1,8000,f); fclose(f); }
    for(int i=0; i<1000; i++) for(int j=0; j<8; j++) if(levelGrid[i][j]>0) maxIdx=i;
    while(1) {
        if(kbhit()){ int k=getch(); if(k==27) break; if(k=='r'||k=='R') reset_player(); }
        if(beep_t>0){ beep_t--; if(beep_t==0) game_beep(0); }
        memset(back_buffer, 15, 64000);
        unsigned char sc = inp(0x60); int hold = (sc==57||sc==17||sc==72||sc==0x48);
        if(!hold) canActivateOrb = 1;
        if(gameState==1) {
            int prg = (oi * 100) / (maxIdx > 0 ? maxIdx : 1);
            if(prg > 100) prg = 100;
            drawRect(60, 5, 200, 4, 7); drawRect(60, 5, prg * 2, 4, 0);
            vy += (0.7*gravityDir); py += vy;
            int on_p=0, inOrb=-1;
            for(int i=0; i<17; i++) {
                int sx=ox+(i*20);
                for(int r=0; r<8; r++) {
                    int t=levelGrid[oi+i][r], sy=r*20+20;
                    if(t==4) {
                        drawRect(sx, sy, 20, 20, 0);
                        if(sx>22 && sx<58) {
                            if(gravityDir>0 && py+18>=sy && py+18<=sy+10 && vy>=0) { py=sy-18; vy=0; on_p=1; angle=(int)(angle+45)/90*90; }
                            else if(gravityDir<0 && py<=sy+20 && py>=sy+10 && vy<=0) { py=sy+20; vy=0; on_p=1; angle=(int)(angle+45)/90*90; }
                            else if(sx>32 && sx<48 && py+16>sy && py<sy+16) reset_player();
                        }
                    } else if(t==1 || t==11) {
                        drawSpike(sx, sy, (t==11));
                        if(sx>35 && sx<45 && py+15>sy && py<sy+15) reset_player();
                    } else if(t>=2 && t<=6 || t==9 || t==12) {
                        int cols[]={14,13,11,10,4,0};
                        int ci = (t==9)?4 : (t==12)?5 : (t==5)?2 : (t==6)?3 : t-2;
                        drawOrb(sx+10, sy+10, cols[ci]);
                        if(sx>10 && sx<70 && py>sy-40 && py<sy+40) {
                            inOrb = (oi+i)*10+r;
                            if(hold && canActivateOrb && lastOrb != inOrb) {
                                if(t==2) vy = -9.0*gravityDir;
                                else if(t==3) vy = -6.0*gravityDir;
                                else if(t==5) { gravityDir *= -1; vy = 1.2*gravityDir; }
                                else if(t==6) { gravityDir *= -1; vy = -9.0*gravityDir; }
                                else if(t==9) vy = -12.5*gravityDir;
                                else if(t==12) vy = 12.0*gravityDir;
                                lastOrb = inOrb; canActivateOrb = 0; game_beep(600); beep_t = 2;
                            }
                        }
                    }
                }
            }
            if(inOrb == -1) lastOrb = -1;
            if(!on_p) {
                if(py>=155 && gravityDir>0) { py=155; vy=0; angle=(int)(angle+45)/90*90; on_p=1; }
                if(py<=20 && gravityDir<0) { py=20; vy=0; angle=(int)(angle+45)/90*90; on_p=1; }
            }
            if(hold && on_p && vy==0) { vy=-8.5*gravityDir; game_beep(400); beep_t=2; }
            if(vy!=0) angle += 12.0*gravityDir;
            drawRotatedCube(40, (int)py, angle);
            ox-=5; if(ox<=-20){ ox+=20; oi++; }
        } else { drawRotatedCube(40, 155, 0); if(hold) gameState=1; }
        while(inp(0x3DA)&8); while(!(inp(0x3DA)&8)); memcpy(video_mem, back_buffer, 64000);
    }
    game_beep(0); __asm__ ("int $0x10"::"a"(3)); return 0;
}
