#ifndef DIAG_CONFIG_H
#define DIAG_CONFIG_H

/*app request flag address*/
#define BOOT_PROGRAM_FLAG_ADDRESS		0x3FFC

/*app have requested*/
#define PRAOGRAM_REQUESTED_FLAG_125K			0x5555AAAA
#define PRAOGRAM_REQUESTED_FLAG_500K			0x5555AA99

#define PRAOGRAM_REQUESTED_FLAG			0x5555AAAA

/*app dont request*/
#define PROGRAM_NO_REQUEST				0xAAAA5555

/*app flag size(in byte)*/
#define PROGRAM_FLAG_SIZE					4

/*app valid flag address*/
#define APP_VALID_FLAG_ADDRESS			0x13F8

/*app flag valid*/
#define APP_IS_VALID						0xA55A

/*app flag invalid*/
#define APP_IS_INVALID						0x5AA5

/*app valid flag size*/
#define APP_VALID_FLAG_SIZE				2

#define HARDWARE_VERSION_ADDRESS              0x13B4

#define BOOTADER_VERSION_ADDRESS               0x13C4

#define SOFTWARE_VERSION_ADDRESS              0x13B6

#define VIN_ADDRESS                                          0x139C

#define SUPPLIER_CODE_ADDRESS                      0x1390

#define PART_NUMBER_ADDRESS                         0x13C0

#define SGMW_HARD_NUM_ADDRESS                   0x13B0

#define SGMW_SOFT_NUM_ADDRESS                   0x138A

#define PROGRAM_SUCCESS_ADDRESS		       0x13EE

#define PROGRAM_ATTEMPT_ADDRESS		       0x13EF

#define SEED_VARAINT_CODE_ADDRESS             0x13F4

#define PROJECT_NAME_ADDRESS                        0x7F7A

#define PROJECT_NAME_LENGTH                          6

#endif