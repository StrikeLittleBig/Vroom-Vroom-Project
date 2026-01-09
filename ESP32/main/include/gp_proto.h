/**
 * @file    gp_proto.h
 * @brief   Protocole d’échange gamepad (format de trame binaire 28 octets).
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   Wire format (sur UDP/SPI) :
 *     - endianness: **little-endian** (ESP32-C3 et PC x86)
 *     - float: IEEE-754 32-bit
 *     - alignement: packed (1 octet)
 *
 *   Structure:
 *     - buttons : bitfield 32 bits
 *     - lx, ly, rx, ry, lt, rt : axes en float
 *
 *   NB: le numéro de port UDP n’est pas une propriété du *protocole* ;
 *       utilisez `CONFIG_GP_UDP_PORT` côté build, ou gardez le fallback ci-dessous.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================== Versioning =============================== */
#define GP_PROTO_VERSION   1u
/* (Optionnel) Magic pour debug capture Wireshark / trames SPI */
#define GP_PROTO_MAGIC     0x47504144u /* 'GPAD' */

/* ================================ Boutons ================================ */
/* Adapte le mapping à ton projet (exemples génériques) */
enum {
    GP_BTN_A      = 1u << 0,
    GP_BTN_B      = 1u << 1,
    GP_BTN_X      = 1u << 2,
    GP_BTN_Y      = 1u << 3,
    GP_BTN_LB     = 1u << 4,
    GP_BTN_RB     = 1u << 5,
    GP_BTN_BACK   = 1u << 6,
    GP_BTN_START  = 1u << 7,
    GP_BTN_LS     = 1u << 8,
    GP_BTN_RS     = 1u << 9,
    /* … jusqu’à 31 */
};

/* ============================== Trame binaire ============================ */
#if defined(_MSC_VER)
  #pragma pack(push, 1)
#endif
typedef struct __attribute__((packed)) {
    uint32_t buttons; /* bitfield (little-endian) */
    float    lx, ly;  /* sticks gauche */
    float    rx, ry;  /* sticks droit  */
    float    lt, rt;  /* triggers      */
} gp_packet_t;
#if defined(_MSC_VER)
  #pragma pack(pop)
#endif

/* Invariants de compilation */
#ifdef __cplusplus
  static_assert(sizeof(float) == 4, "gp_proto: float must be 32-bit");
  static_assert(sizeof(gp_packet_t) == 28, "gp_packet_t must be 28 bytes");
#else
  _Static_assert(sizeof(float) == 4, "gp_proto: float must be 32-bit");
  _Static_assert(sizeof(gp_packet_t) == 28, "gp_packet_t must be 28 bytes");
#endif

/* ============================= Helpers utiles ============================ */
static inline void gp_packet_zero(gp_packet_t *p) { memset(p, 0, sizeof(*p)); }

/* Sanity-check simple (ex: axes attendus dans [-1,1], triggers [0,1]) */
static inline bool gp_packet_is_plausible(const gp_packet_t *p) {
    const bool sticks_ok =
        (p->lx >= -1.1f && p->lx <= 1.1f) &&
        (p->ly >= -1.1f && p->ly <= 1.1f) &&
        (p->rx >= -1.1f && p->rx <= 1.1f) &&
        (p->ry >= -1.1f && p->ry <= 1.1f);
    const bool trig_ok = (p->lt >= -0.1f && p->lt <= 1.1f) &&
                         (p->rt >= -0.1f && p->rt <= 1.1f);
    (void)trig_ok; /* si tu n’utilises pas, enlève la ligne ci-dessus */
    return sticks_ok /* && trig_ok */;
}

/* =========================== Paramètres réseau =========================== */
/* Idéal: définir CONFIG_GP_UDP_PORT dans sdkconfig. Fallback sinon. */
#ifndef CONFIG_GP_UDP_PORT
  #define CONFIG_GP_UDP_PORT  5555
#endif
#define GP_UDP_PORT  CONFIG_GP_UDP_PORT

#ifdef __cplusplus
}
#endif
