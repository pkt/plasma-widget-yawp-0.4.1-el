#!/bin/bash

SUDO_PROG="sudo"
#SUDO_PROG="su -c"


ERROR="\033[31m"
WARNING="\033[33m"
MESSAGE="\033[36m"
NORMALTEXT="\033[m"


function rootPrivilegedAction
{
  Action=$1
  USERNAME=`whoami`
  echo "Username: "$USERNAME
  
  echo -e $WARNING"Selected action ($Action) needs root-privileges.\nPlease enter root-password. "$NORMALTEXT
  if [ "$SUDO_PROG" = "sudo" ]; then
    sudo $Action
  else
    eval "su -c '$Action'"
  fi
}

function setSudoProgram
{
  LeaveMenu=0
  while [ $LeaveMenu -eq 0 ]
  do
    echo -e "\n"$MESSAGE"You need root privileges to install the applet or to create a package."
    echo -e "Select the program you use to gain root privileges on your system."$NORMALTEXT
    echo "   1) sudo"
    echo "   2) su"
    echo -n "Choose a number [1]: "
    read Input
    if [ -z $Input ]; then Input=1; fi
    
    case $Input in
      1)	SUDO_PROG="sudo"
		LeaveMenu=1
		;;
      2)	SUDO_PROG="su"
		LeaveMenu=1
		;;
      *)	echo -e $ERROR"Invalid input!"$NORMALTEXT
		;;
    esac
  done
}

function packageMenu
{
  changeRootPrivilege

  LeaveMenu=0
  while [ $LeaveMenu -eq 0 ]
  do
    echo -e "\n"$MESSAGE"Select the package you want to create:"$NORMALTEXT
    echo "   1) Debian packages (DEB)"
    echo "   2) RedHat package (RPM)"
    echo "   3) Self extracting Tar GZip compressed packages (STGZ)"
    echo "   4) Tar BZip2 compressed packages (TBZ2)"
    echo "   5) Tar GZip compressed packages (TGZ)"
    echo "   6) Tar compressed packages (TZ)"
    echo "   7) Zip package (ZIP)"
    echo "   8) Exit"
    echo -n "Choose a number [8]: "
    read Input
    if [ -z $Input ]; then Input=8; fi
    
    case $Input in
      1)	echo -e "\nCreate DEB-Package"
		rootPrivilegedAction "cpack -G DEB --config CPackConfig.cmake"
		;;
      2)	echo -e "\nCreate RPM-Package"
		rootPrivilegedAction "cpack -G RPM --config CPackConfig.cmake"
		;;
      3)	echo -e "\nCreate STGZ-Package"
		rootPrivilegedAction "cpack -G STGZ --config CPackConfig.cmake"
		;;
      4)	echo -e "\nCreate TBZ2-Package"
		rootPrivilegedAction "cpack -G TBZ2 --config CPackConfig.cmake"
		;;
      5)	echo -e "\nCreate TGZ-Package"
		rootPrivilegedAction "cpack -G TGZ --config CPackConfig.cmake"
		;;
      6)	echo -e "\nCreate TAR-Package"
		rootPrivilegedAction "cpack -G TZ --config CPackConfig.cmake"
		;;
      7)	echo -e "\nCreate ZIP-Package"
		rootPrivilegedAction "cpack -G ZIP --config CPackConfig.cmake"
		;;
      8)	LeaveMenu=1
		;;
      *)	echo -e "\n"$ERROR"Invalid input!"$NORMALTEXT
		;;
    esac
  done

  cd ..
}

function installBinaries
{
  # Do not enter to the build directory when we are already inside
  [ "$(pwd |grep -c '\/build$')" -ne 1 ] && cd build
  rootPrivilegedAction "make install"
  cd ..
}

function mainMenu
{
  while [ 1 ]
  do
    echo -e "\n"$MESSAGE"What do you want to do?"$NORMALTEXT
    echo "   1) Create a distribution package."
    echo "   2) Install files from build directory."
    echo "   3) Install files from build directory and Exit"
    echo "   4) Exit"
    echo -n "Choose a number [3]: "
    read Input
    if [ -z $Input ]; then Input=3; fi

    case $Input in
      1)  packageMenu
        ;;
      2)  installBinaries
        ;;
      3)  installBinaries
          echo "Good-by..."
          exit 0
        ;;
      4)  echo "Good-by..."
          exit 0
        ;;
      *) echo -e $ERROR"Invalid input!"$NORMALTEXT
        ;;
    esac
  done
echo "Scheme: "$scheme
}

function usage
{
  echo -e "usage: $0 [-d] [-f LOGFILE] [-l LOGLEVEL] [-u] [-h]\n"
  echo -e "Compile and install yaWP KDE plasmoid\n"
  echo "OPTIONS:"
  echo "   -h        print this help and exit"
  echo "   -d        build with debug informations"
  echo "   -f FILE   write all yawp logmessages to specified file;"
  echo "              '-f stdout' or '-f stderr' will direct the"
  echo "               logmessages to the terminal."
  echo "   -l LEVEL  logmessages with less priority will be"
  echo "              suppressed; loglevels with increasing priority:"
  echo "              [Tracing,Debug,Info,Warning,Critical,Error]"
  echo "   -u        build unittest directory"
  echo "   -j CORES  Number of CPUs to use for compilation"
  echo "              Default: 2 cores on Dual core CPU"
  echo "                       (CPU cores - 1) for Multi-cores CPU (>2)"
}

##############################################################################################################

## Parse command lines arguments ## 
DEBUG="-DCMAKE_BUILD_TYPE=Release"
UNITTESTS=
LOGFILE="-DDEBUG_LOGFILE=/tmp/yawp.log"
LOGLEVEL="-DDEBUG_LOGLEVEL=Debug"

while getopts "hduf:l:j:" OPT
do
	case $OPT in
	d) DEBUG="-DCMAKE_BUILD_TYPE=Debug" ;;
	u) UNITTESTS="-DBUILD_UNITTESTS=YES" ;;
	f) LOGFILE="-DDEBUG_LOGFILE=$OPTARG" ;;
	l) case "$OPTARG" in
		'Tracing')	LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		'Debug')	LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		'Info')		LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		'Warning')	LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		'Critical')	LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		'Error')	LOGLEVEL="-DDEBUG_LOGLEVEL=$OPTARG" ;;
		*)  echo "$0: invalid argument for option -- l"
			usage ; exit ;;
		esac ;;
	j) JOB_COUNT=$OPTARG ;;
	?) usage ; exit ;;
	esac

done

echo "DEBUG: $DEBUG"
echo "UNITTESTS: $UNITTESTS"
echo "LOGLEVEL: $LOGLEVEL"
echo "LOGFILE: $LOGFILE"
echo
echo "Prepare build directory..."
rm --recursive --force build CMakeFiles CMakeTmp CMakeCache.txt cmake_install.cmake cmake_uninstall.cmake CTestTestfile.cmake Makefile

if [ ! -d build  ]; then
	mkdir build
fi

ret=$?
if [ $ret -ne 0 ]; then
	echo -e $ERROR"Could not create build directory. Check your rights for this directory."$NORMALTEXT
	exit
fi

echo 
echo "Run cmake..."
cd build
cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` $DEBUG $UNITTESTS $LOGFILE $LOGLEVEL ..

ret=$?
if [ $ret -ne 0 ]; then
	echo -e $ERROR"CMake failed. Your system probably does not meets all requirements."$NORMALTEXT
	exit
fi

echo
echo "Run make clean..."
make clean

echo
echo "Compile..."
if [ -z "$JOB_COUNT" ]; then
	## Try to determine the number of processor-cores.
	JOB_COUNT=1
	if [ -f /proc/cpuinfo ]; then
		JOB_COUNT=`grep ^processor /proc/cpuinfo | wc -l`
	fi

	## Just make sure, we have a valid number (e.g.: in case we have no rights to read the processor cores).
	## Can this happen??
	if [ $JOB_COUNT -eq 0 ]; then
		JOB_COUNT=1
	fi

	## Use almost all processor cores, when we have more than two cores.
	## We are not too selfish and leave at least one core for other programs, when user has more than two cores :)
	## Just to optimise the compile time and do not let the user wait to long for new applet.
	if [ $JOB_COUNT -gt 2 ]; then
		let "JOB_COUNT = ($JOB_COUNT - 1)"
	fi
fi

echo -e $MESSAGE"Going to use $JOB_COUNT cores to compile package yaWP."$NORMALTEXT
make -j $JOB_COUNT

ret=$?
if [ $ret -ne 0 ]; then
	echo -e $ERROR"Compilation failed, sorry :-("$NORMALTEXT
	exit
fi

setSudoProgram
mainMenu
