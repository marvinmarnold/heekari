###########################################################
# Makefile generated by xIDE for uEnergy                   
#                                                          
# Project: hr_sensor
# Configuration: Release
# Generated: Wed May 7 22:29:48 2014
#                                                          
# WARNING: Do not edit this file. Any changes will be lost 
#          when the project is rebuilt.                    
#                                                          
###########################################################

XIDE_PROJECT=hr_sensor
XIDE_CONFIG=Release
OUTPUT=hr_sensor
OUTDIR=C:/Users/pili/git/heekari/HeekariFirmware
DEFS=

OUTPUT_TYPE=0
LIBRARY_VERSION=Auto
SWAP_INTO_DATA=0
USE_FLASH=0
ERASE_NVM=1
CSFILE_CSR100x=hr_sensor_csr100x.keyr
CSFILE_CSR101x_A05=hr_sensor_csr101x_A05.keyr
MASTER_DB=app_gatt_db.db
LIBPATHS=
INCPATHS=

DBS=\
\
      app_gatt_db.db\
      battery_service_db.db\
      dev_info_service_db.db\
      gap_service_db.db\
      gatt_service_db.db\
      heart_rate_service_db.db\
      switch_service_db.db

INPUTS=\
      battery_service.c\
      gap_service.c\
      heart_rate_service.c\
      hr_sensor.c\
      hr_sensor_gatt.c\
      hr_sensor_hw.c\
      nvm_access.c\
      switch_service.c\
      $(DBS)

KEYR=\
      hr_sensor_csr100x.keyr\
      hr_sensor_csr101x_A05.keyr


-include hr_sensor.mak
include $(SDK)/genmakefile.uenergy
