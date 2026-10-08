/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef QCE_COLOR_H
#define QCE_COLOR_H
static qboolean QCE_ParseRGB(const char *s, byte out[4]) {
 int i,value,n;byte result[4];
 for(i=0;i<3;i++) {
  while(*s==' ' || *s=='\t')s++;
  value=0;n=0;
  while(*s>='0' && *s<='9') {value=value*10+(*s++-'0');if(++n>3 || value>255)return qfalse;}
  if(!n || (i<2 && *s!=' ' && *s!='\t'))return qfalse;
  result[i]=(byte)value;
 }
 while(*s==' ' || *s=='\t')s++;
 if(*s)return qfalse;
 result[3]=255;memcpy(out,result,4);return qtrue;
}
#endif
