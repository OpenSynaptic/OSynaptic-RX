/*
 * osrx_units.h -- OpenSynaptic unit wire-code constants for OSynaptic-RX.
 *
 * Every macro expands to the OS_Symbols.json wire code for that unit.
 * These are the same codes that OSynaptic-TX writes into the packet body;
 * use them to compare against osrx_sensor_field.unit without relying on
 * human-readable strings.
 *
 * USAGE
 * -----
 *   static void on_frame(const osrx_packet_meta  *meta,
 *                        const osrx_sensor_field *field, ...)
 *   {
 *       if (!field) return;
 *       if (strcmp(field->unit, OSRX_UNIT(Cel)) == 0) {
 *           long whole = (long)(field->scaled / OSRX_VALUE_SCALE);
 *           long frac  = (long)(field->scaled % OSRX_VALUE_SCALE);
 *           if (frac < 0) frac = -frac;
 *           Serial.print(whole); Serial.print('.'); Serial.println(frac);
 *       }
 *   }
 *
 * COMPILE-TIME VALIDATION
 * -----------------------
 * OSRX_UNIT(sym) expands to OSRX_UNIT_##sym.
 * An unknown sym causes a compile error:
 *   error: 'OSRX_UNIT_Celsius' undeclared
 *
 * SOURCES
 * -------
 * Wire codes derived from OpenSynaptic OS_Symbols.json v1.1.0.
 * Must be kept in sync with OSynaptic-TX ostx_units.h.
 */

#ifndef OSRX_UNITS_H
#define OSRX_UNITS_H

/*
 * Convenience macro -- expands sym to its wire-code string literal.
 */
#define OSRX_UNIT(sym)  OSRX_UNIT_##sym

/* =========================================================================
 * CLASS 6 -- Length  (base: m = "600")
 * ========================================================================= */
#define OSRX_UNIT_m      "600"      /* meter            (SI, can prefix) */
#define OSRX_UNIT_in     "601"      /* inch */
#define OSRX_UNIT_ft     "602"      /* foot */
#define OSRX_UNIT_nmi    "603"      /* nautical mile */
#define OSRX_UNIT_AU     "604"      /* astronomical unit */
#define OSRX_UNIT_ang    "605"      /* angstrom */

/* =========================================================================
 * CLASS 8 -- Mass  (base: g = "800")
 * ========================================================================= */
#define OSRX_UNIT_g      "800"      /* gram              (SI, can prefix) */
#define OSRX_UNIT_lb     "801"      /* pound */
#define OSRX_UNIT_oz     "802"      /* ounce */
#define OSRX_UNIT_t      "803"      /* metric ton        (can prefix) */
#define OSRX_UNIT_u      "804"      /* unified atomic mass unit */

/* =========================================================================
 * CLASS B -- Time  (base: s = "B00")
 * ========================================================================= */
#define OSRX_UNIT_s      "B00"      /* second            (SI, can prefix) */
#define OSRX_UNIT_min    "B01"      /* minute */
#define OSRX_UNIT_h      "B02"      /* hour */
#define OSRX_UNIT_d      "B03"      /* day */
#define OSRX_UNIT_wk     "B04"      /* week */
#define OSRX_UNIT_ann    "B05"      /* year              (can prefix) */

/* =========================================================================
 * CLASS A -- Temperature  (base: K = "A00")
 * ========================================================================= */
#define OSRX_UNIT_K      "A00"      /* kelvin            (SI, can prefix) */
#define OSRX_UNIT_Cel    "A01"      /* degree Celsius    (OS: cel) */
#define OSRX_UNIT_degF   "A02"      /* degree Fahrenheit (OS: degf) */
#define OSRX_UNIT_degRe  "A03"      /* degree Reaumur    (OS: degre) */

/* =========================================================================
 * CLASS 1 -- Electric Current  (base: A = "100")
 * ========================================================================= */
#define OSRX_UNIT_A      "100"      /* ampere            (SI, can prefix) */
#define OSRX_UNIT_Bi     "101"      /* biot              (can prefix) */
#define OSRX_UNIT_Gau    "102"      /* gauss unit */

/* =========================================================================
 * CLASS 0 -- Amount of Substance  (base: mol = "000")
 * ========================================================================= */
#define OSRX_UNIT_mol    "000"      /* mole              (SI, can prefix) */
#define OSRX_UNIT_eq     "001"      /* equivalents       (can prefix) */
#define OSRX_UNIT_osm    "002"      /* osmole            (can prefix) */
#define OSRX_UNIT_count  "003"      /* particle count */

/* =========================================================================
 * CLASS 7 -- Luminous Intensity  (base: cd = "700")
 * ========================================================================= */
#define OSRX_UNIT_cd     "700"      /* candela           (SI, can prefix) */
#define OSRX_UNIT_cp     "701"      /* candlepower */
#define OSRX_UNIT_hk     "702"      /* hefnerkerze */

/* =========================================================================
 * CLASS 9 -- Pressure  (base: Pa = "900")
 * ========================================================================= */
#define OSRX_UNIT_Pa     "900"      /* pascal            (SI, can prefix) */
#define OSRX_UNIT_bar    "901"      /* bar               (can prefix) */
#define OSRX_UNIT_psi    "902"      /* pound-force per square inch */
#define OSRX_UNIT_atm    "900"      /* standard atmosphere (same code as Pa) */
#define OSRX_UNIT_mmHg   "903"      /* millimeter of mercury (OS: mm[hg]) */

/* =========================================================================
 * CLASS 4 -- Frequency  (base: Hz = "400")
 * ========================================================================= */
#define OSRX_UNIT_Hz     "400"      /* hertz             (SI, can prefix) */
#define OSRX_UNIT_rpm    "401"      /* revolutions per minute */
#define OSRX_UNIT_deg_s  "402"      /* degrees per second  (OS: deg/s) */
#define OSRX_UNIT_rad_s  "403"      /* radians per second  (OS: rad/s) */

/* =========================================================================
 * CLASS 3 -- Energy / Power  (base: W = "300")
 * ========================================================================= */
#define OSRX_UNIT_W      "300"      /* watt              (SI, can prefix) */
#define OSRX_UNIT_J      "301"      /* joule             (SI, can prefix) */
#define OSRX_UNIT_cal    "302"      /* calorie           (can prefix) */
#define OSRX_UNIT_hp     "303"      /* horsepower */

/* =========================================================================
 * CLASS 2 -- Electromagnetism  (base: V = "200")
 * ========================================================================= */
#define OSRX_UNIT_V      "200"      /* volt              (SI, can prefix) */
#define OSRX_UNIT_Ohm    "201"      /* ohm               (SI, can prefix) */
#define OSRX_UNIT_F      "202"      /* farad             (SI, can prefix) */

/* =========================================================================
 * CLASS 5 -- Informatics  (base: bit = "500")
 * ========================================================================= */
#define OSRX_UNIT_bit    "500"      /* bit               (can prefix) */
#define OSRX_UNIT_By     "501"      /* byte              (can prefix) */
#define OSRX_UNIT_Bd     "502"      /* baud              (can prefix) */

/* =========================================================================
 * CLASS C -- Humidity  (base: % = "C00")
 * ========================================================================= */
#define OSRX_UNIT_pct    "C00"      /* relative humidity percent (OS: %) */
#define OSRX_UNIT_RH     "C00"      /* alias: relative humidity */

/* =========================================================================
 * CLASS D -- Device Operations
 * =========================================================================
 * "." in OS names is mapped to "_" in the macro suffix.
 */
#define OSRX_UNIT_cmd      "D00"
#define OSRX_UNIT_pow_on   "D01"
#define OSRX_UNIT_pow_off  "D02"
#define OSRX_UNIT_set_val  "D03"
#define OSRX_UNIT_get_val  "D04"
#define OSRX_UNIT_get_st   "D05"
#define OSRX_UNIT_rst      "D06"
#define OSRX_UNIT_mv_up    "D07"
#define OSRX_UNIT_mv_dn    "D08"
#define OSRX_UNIT_mv_lt    "D09"
#define OSRX_UNIT_mv_rt    "D0A"
#define OSRX_UNIT_mv_fw    "D0B"
#define OSRX_UNIT_mv_bk    "D0C"
#define OSRX_UNIT_stp      "D0D"
#define OSRX_UNIT_stp_e    "D0E"
#define OSRX_UNIT_mv_to    "D0F"
#define OSRX_UNIT_mv_by    "D10"
#define OSRX_UNIT_rot_cw   "D11"
#define OSRX_UNIT_rot_cc   "D12"

/* Custom command slots A-Z (D13 -- D2C) */
#define OSRX_UNIT_cmdA   "D13"
#define OSRX_UNIT_cmdB   "D14"
#define OSRX_UNIT_cmdC   "D15"
#define OSRX_UNIT_cmdD   "D16"
#define OSRX_UNIT_cmdE   "D17"
#define OSRX_UNIT_cmdF   "D18"
#define OSRX_UNIT_cmdG   "D19"
#define OSRX_UNIT_cmdH   "D1A"
#define OSRX_UNIT_cmdI   "D1B"
#define OSRX_UNIT_cmdJ   "D1C"
#define OSRX_UNIT_cmdK   "D1D"
#define OSRX_UNIT_cmdL   "D1E"
#define OSRX_UNIT_cmdM   "D1F"
#define OSRX_UNIT_cmdN   "D20"
#define OSRX_UNIT_cmdO   "D21"
#define OSRX_UNIT_cmdP   "D22"
#define OSRX_UNIT_cmdQ   "D23"
#define OSRX_UNIT_cmdR   "D24"
#define OSRX_UNIT_cmdS   "D25"
#define OSRX_UNIT_cmdT   "D26"
#define OSRX_UNIT_cmdU   "D27"
#define OSRX_UNIT_cmdV   "D28"
#define OSRX_UNIT_cmdW   "D29"
#define OSRX_UNIT_cmdX   "D2A"
#define OSRX_UNIT_cmdY   "D2B"
#define OSRX_UNIT_cmdZ   "D2C"

/* Mode switch slots A-Z (D2D -- D46) */
#define OSRX_UNIT_modeA  "D2D"
#define OSRX_UNIT_modeB  "D2E"
#define OSRX_UNIT_modeC  "D2F"
#define OSRX_UNIT_modeD  "D30"
#define OSRX_UNIT_modeE  "D31"
#define OSRX_UNIT_modeF  "D32"
#define OSRX_UNIT_modeG  "D33"
#define OSRX_UNIT_modeH  "D34"
#define OSRX_UNIT_modeI  "D35"
#define OSRX_UNIT_modeJ  "D36"
#define OSRX_UNIT_modeK  "D37"
#define OSRX_UNIT_modeL  "D38"
#define OSRX_UNIT_modeM  "D39"
#define OSRX_UNIT_modeN  "D3A"
#define OSRX_UNIT_modeO  "D3B"
#define OSRX_UNIT_modeP  "D3C"
#define OSRX_UNIT_modeQ  "D3D"
#define OSRX_UNIT_modeR  "D3E"
#define OSRX_UNIT_modeS  "D3F"
#define OSRX_UNIT_modeT  "D40"
#define OSRX_UNIT_modeU  "D41"
#define OSRX_UNIT_modeV  "D42"
#define OSRX_UNIT_modeW  "D43"
#define OSRX_UNIT_modeX  "D44"
#define OSRX_UNIT_modeY  "D45"
#define OSRX_UNIT_modeZ  "D46"

#endif /* OSRX_UNITS_H */
