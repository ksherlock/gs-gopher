#pragma optimize 79
#pragma noroot

#include <Types.h>
#include "url.h"

void parse_scheme(const char *cp, unsigned size, URLComponents *c)
{
  unsigned *wp;
  unsigned h;

  if (!c) return;
  if (!cp || !size)
  {
    c->portNumber = 0;
    c->schemeType = SCHEME_NONE;
    return;
  }

  wp = (unsigned *)cp;
  h = ((*cp | 0x20) + size) & 0x0f;

  switch(h)
  {
  // --- begin auto-generated --
    case 0x01:
      // nfs
      if (size == 3
        && (wp[0] | 0x2020) == 0x666e // 'nf'
        && (cp[2] | 0x20) == 0x73     // 's'
      ) {
        c->schemeType = SCHEME_NFS;
        c->portNumber = 2049;
        return;
      }
      break;

    case 0x02:
      // nntp
      if (size == 4
        && (wp[0] | 0x2020) == 0x6e6e // 'nn'
        && (wp[1] | 0x2020) == 0x7074 // 'tp'
      ) {
        c->schemeType = SCHEME_NNTP;
        c->portNumber = 119;
        return;
      }
      break;

    case 0x04:
      // afp
      if (size == 3
        && (wp[0] | 0x2020) == 0x6661 // 'af'
        && (cp[2] | 0x20) == 0x70     // 'p'
      ) {
        c->schemeType = SCHEME_AFP;
        c->portNumber = 548;
        return;
      }
      break;

    case 0x06:
      // ssh
      if (size == 3
        && (wp[0] | 0x2020) == 0x7373 // 'ss'
        && (cp[2] | 0x20) == 0x68     // 'h'
      ) {
        c->schemeType = SCHEME_SSH;
        c->portNumber = 22;
        return;
      }
      // smb
      if (size == 3
        && (wp[0] | 0x2020) == 0x6d73 // 'sm'
        && (cp[2] | 0x20) == 0x62     // 'b'
      ) {
        c->schemeType = SCHEME_SMB;
        c->portNumber = 445;
        return;
      }
      break;

    case 0x07:
      // sftp
      if (size == 4
        && (wp[0] | 0x2020) == 0x6673 // 'sf'
        && (wp[1] | 0x2020) == 0x7074 // 'tp'
      ) {
        c->schemeType = SCHEME_SFTP;
        c->portNumber = 115;
        return;
      }
      break;

    case 0x08:
      // dict
      if (size == 4
        && (wp[0] | 0x2020) == 0x6964 // 'di'
        && (wp[1] | 0x2020) == 0x7463 // 'ct'
      ) {
        c->schemeType = SCHEME_DICT;
        c->portNumber = 2628;
        return;
      }
      // tftp
      if (size == 4
        && (wp[0] | 0x2020) == 0x6674 // 'tf'
        && (wp[1] | 0x2020) == 0x7074 // 'tp'
      ) {
        c->schemeType = SCHEME_TFTP;
        c->portNumber = 69;
        return;
      }
      break;

    case 0x09:
      // ftp
      if (size == 3
        && (wp[0] | 0x2020) == 0x7466 // 'ft'
        && (cp[2] | 0x20) == 0x70     // 'p'
      ) {
        c->schemeType = SCHEME_FTP;
        c->portNumber = 21;
        return;
      }
      break;

    case 0x0a:
      // file
      if (size == 4
        && (wp[0] | 0x2020) == 0x6966 // 'fi'
        && (wp[1] | 0x2020) == 0x656c // 'le'
      ) {
        c->schemeType = SCHEME_FILE;
        c->portNumber = 0;
        return;
      }
      // telnet
      if (size == 6
        && (wp[0] | 0x2020) == 0x6574 // 'te'
        && (wp[1] | 0x2020) == 0x6e6c // 'ln'
        && (wp[2] | 0x2020) == 0x7465 // 'et'
      ) {
        c->schemeType = SCHEME_TELNET;
        c->portNumber = 23;
        return;
      }
      break;

    case 0x0c:
      // http
      if (size == 4
        && (wp[0] | 0x2020) == 0x7468 // 'ht'
        && (wp[1] | 0x2020) == 0x7074 // 'tp'
      ) {
        c->schemeType = SCHEME_HTTP;
        c->portNumber = 80;
        return;
      }
      break;

    case 0x0d:
      // gopher
      if (size == 6
        && (wp[0] | 0x2020) == 0x6f67 // 'go'
        && (wp[1] | 0x2020) == 0x6870 // 'ph'
        && (wp[2] | 0x2020) == 0x7265 // 'er'
      ) {
        c->schemeType = SCHEME_GOPHER;
        c->portNumber = 70;
        return;
      }
      // https
      if (size == 5
        && (wp[0] | 0x2020) == 0x7468 // 'ht'
        && (wp[1] | 0x2020) == 0x7074 // 'tp'
        && (cp[4] | 0x20) == 0x73     // 's'
      ) {
        c->schemeType = SCHEME_HTTPS;
        c->portNumber = 443;
        return;
      }
      break;

  // --- end auto-generated --
  }

  c->portNumber = 0;
  c->schemeType = SCHEME_UNKNOWN;
}
