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
 *******************************************************************************/

#include <string.h>

#include "connect.h"
#include "helpers.h"

typedef union {
  unsigned char byte;

  struct {
    unsigned int _ : 1;
    unsigned int clean_session : 1;
    unsigned int will : 1;
    unsigned int will_qos : 2;
    unsigned int will_retain : 1;
    unsigned int password : 1;
    unsigned int username : 1;
  } bits;
} lwmqtt_connect_flags_t;

typedef union {
  unsigned char byte;

  struct {
    unsigned int _ : 7;
    unsigned int session_present : 1;
  } bits;
} lwmqtt_connack_flags_t;

int lwmqtt_serialize_connect(unsigned char *buf, int buf_len, lwmqtt_options_t *options, lwmqtt_will_t *will) {
  // prepare pointer
  unsigned char *ptr = buf;

  /* calculate remaining length */

  // fixed header is 10
  int rem_len = 10;

  // add client id
  rem_len += lwmqtt_strlen(options->client_id) + 2;

  // add will if present
  if (will != NULL) {
    rem_len += lwmqtt_strlen(will->topic) + 2 + will->payload_len + 2;
  }

  // add username if present
  if (options->username.c_string || options->username.lp_string.data) {
    rem_len += lwmqtt_strlen(options->username) + 2;

    // add password if present
    if (options->password.c_string || options->password.lp_string.data) {
      rem_len += lwmqtt_strlen(options->password) + 2;
    }
  }

  // check buffer capacity
  if (lwmqtt_total_header_length(rem_len) + rem_len > buf_len) {
    return LWMQTT_BUFFER_TOO_SHORT_ERROR;
  }

  /* serialize packet */

  // write header
  lwmqtt_header_t header = {0};
  header.bits.type = LWMQTT_CONNECT_PACKET;
  lwmqtt_write_char(&ptr, header.byte);

  // write remaining length
  ptr += lwmqtt_encode_remaining_length(ptr, rem_len);

  // write version
  lwmqtt_write_c_string(&ptr, "MQTT");
  lwmqtt_write_char(&ptr, 4);

  // prepare flags
  lwmqtt_connect_flags_t flags = {0};
  flags.bits.clean_session = options->clean_session;

  // set will flags if present
  if (will != NULL) {
    flags.bits.will = 1;
    flags.bits.will_qos = (unsigned int)will->qos;
    flags.bits.will_retain = will->retained;
  }

  // set username flag if present
  if (options->username.c_string || options->username.lp_string.data) {
    flags.bits.username = 1;

    // set password flag if present
    if (options->password.c_string || options->password.lp_string.data) {
      flags.bits.password = 1;
    }
  }

  // write flags
  lwmqtt_write_char(&ptr, flags.byte);

  // write keep alive
  lwmqtt_write_int(&ptr, options->keep_alive);

  // write client id
  lwmqtt_write_string(&ptr, options->client_id);

  // write will topic and payload if present
  if (will != NULL) {
    lwmqtt_write_string(&ptr, will->topic);
    lwmqtt_write_int(&ptr, (int)will->payload_len);
    memcpy(ptr, will->payload, will->payload_len);
    ptr += will->payload_len;
  }

  // write username if present
  if (flags.bits.username) {
    lwmqtt_write_string(&ptr, options->username);

    // write password if present
    if (flags.bits.password) {
      lwmqtt_write_string(&ptr, options->password);
    }
  }

  // return written length
  return (int)(ptr - buf);
}

int lwmqtt_deserialize_connack(bool *session_present, lwmqtt_connack_t *connack_rc, unsigned char *buf, int buf_len) {
  // prepare pointer
  unsigned char *ptr = buf;

  // read header
  lwmqtt_header_t header;
  header.byte = lwmqtt_read_char(&ptr);
  if (header.bits.type != LWMQTT_CONNACK_PACKET) {
    return 0;
  }

  // read remaining length
  int len;
  int rc = lwmqtt_decode_remaining_length(ptr, &len);
  if (rc == LWMQTT_HEADER_DECODE_ERROR) {
    return 0;
  }

  // check lengths
  if (len != 2 || len + 2 < buf_len) {
    return 0;
  }

  // advance pointer
  ptr++;

  // read flags
  lwmqtt_connack_flags_t flags;
  flags.byte = lwmqtt_read_char(&ptr);
  *session_present = flags.bits.session_present == 1;
  *connack_rc = (lwmqtt_connack_t)lwmqtt_read_char(&ptr);

  return 1;
}

static int lwmqtt_serialize_zero(unsigned char *buf, int buf_len, unsigned char packet_type) {
  lwmqtt_header_t header = {0};
  unsigned char *ptr = buf;

  if (buf_len < 2) {
    return LWMQTT_BUFFER_TOO_SHORT_ERROR;
  }

  header.byte = 0;
  header.bits.type = packet_type;
  lwmqtt_write_char(&ptr, header.byte);  // write header

  ptr += lwmqtt_encode_remaining_length(ptr, 0);  // write remaining length

  return (int)(ptr - buf);
}

int lwmqtt_serialize_disconnect(unsigned char *buf, int buf_len) {
  return lwmqtt_serialize_zero(buf, buf_len, LWMQTT_DISCONNECT_PACKET);
}

int lwmqtt_serialize_pingreq(unsigned char *buf, int buf_len) {
  return lwmqtt_serialize_zero(buf, buf_len, LWMQTT_PINGREQ_PACKET);
}
