/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - non-blocking packet read functions for stream transport
 *******************************************************************************/

#include "packet.h"

int lwmqtt_fixed_header_encode(unsigned char *buf, int length) {
  int rc = 0;

  do {
    char d = length % 128;
    length /= 128;
    // if there are more digits to encode, set the top bit of this digit
    if (length > 0) d |= 0x80;
    buf[rc++] = d;
  } while (length > 0);

  return rc;
}

int lwmqtt_fixed_header_len(int rem_len) {
  rem_len += 1;  // header byte

  // now remaining_length field
  if (rem_len < 128) {
    rem_len += 1;
  } else if (rem_len < 16384) {
    rem_len += 2;
  } else if (rem_len < 2097151) {
    rem_len += 3;
  } else {
    rem_len += 4;
  }

  return rem_len;
}

int lwmqtt_fixed_header_decode(unsigned char *buf, int *value) {
  unsigned char c;
  int multiplier = 1;
  int len = 0;

  *value = 0;
  do {
    len++;

    if (len > 4) {
      return LWMQTT_READ_ERROR;  // bad data
    }

    c = buf[len - 1];

    *value += (c & 127) * multiplier;
    multiplier *= 128;
  } while ((c & 128) != 0);

  return len;
}

/* helpers */

int lwmqtt_read_int(unsigned char **pptr) {
  // get pointer
  unsigned char *ptr = *pptr;

  // read two byte integer
  int num = 256 * (*ptr) + (*(ptr + 1));

  // adjust pointer
  *pptr += 2;

  return num;
}

char lwmqtt_read_char(unsigned char **pptr) {
  // read single char
  char chr = **pptr;

  // adjust pointer
  (*pptr)++;

  return chr;
}

void lwmqtt_write_char(unsigned char **pptr, unsigned char chr) {
  // write single char
  **pptr = chr;

  // adjust pointer
  (*pptr)++;
}

void lwmqtt_write_int(unsigned char **pptr, int num) {
  // write first byte
  **pptr = (unsigned char)(num / 256);

  // adjust pointer
  (*pptr)++;

  // write second byte
  **pptr = (unsigned char)(num % 256);

  // adjust pointer
  (*pptr)++;
}
