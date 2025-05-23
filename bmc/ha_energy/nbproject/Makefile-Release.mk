#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/5c0/energy.o \
	${OBJECTDIR}/bsoc.o \
	${OBJECTDIR}/http_vars.o \
	${OBJECTDIR}/mqtt_rec.o \
	${OBJECTDIR}/mqtt_vars.o \
	${OBJECTDIR}/pid.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs libcjson` -lpaho-mqtt3c -lm  `pkg-config --libs libcurl`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/ha_energy

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/ha_energy: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/ha_energy ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/5c0/energy.o: ../energy.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/energy.o ../energy.c

${OBJECTDIR}/bsoc.o: bsoc.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/bsoc.o bsoc.c

${OBJECTDIR}/http_vars.o: http_vars.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/http_vars.o http_vars.c

${OBJECTDIR}/mqtt_rec.o: mqtt_rec.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/mqtt_rec.o mqtt_rec.c

${OBJECTDIR}/mqtt_vars.o: mqtt_vars.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/mqtt_vars.o mqtt_vars.c

${OBJECTDIR}/pid.o: pid.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O3 -Wall -s `pkg-config --cflags libcjson` `pkg-config --cflags libcurl` -std=c11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/pid.o pid.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
