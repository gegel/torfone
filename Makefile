#system depened libs
ifdef SYSTEMROOT
#Win32    
TFLIBS= -lcomctl32 -lwinmm -lws2_32
else
   ifeq ($(shell uname), Linux)
#Linux     
TFLIBS= -lasound -lm
   endif
endif

#compiller
CC = gcc -Wall -O2
#compiller flags (not used yet)
CFLAGS = -Isnd -Isnd/dev -Isnd/npp -Isnd/amr -Itransport -Itransport/obflib -Istorage -Istorage/rndlib -Icryptor -Icryptor/crplib -Iecc -Iuser
#-fomit-frame-pointer -ffast-math -funroll-loops
#including pathes
TFPATH = -Isnd -Isnd/dev -Isnd/npp -Isnd/amr -Itransport -Itransport/obflib -Istorage -Istorage/rndlib -Icryptor -Icryptor/crplib -Iecc -Iuser

#============================Source files===============================
#user interface
USRFILES = user/ui.c user/ui_if.c user/whereami.c user/telnet.c
#storage
STFILES = storage/st.c storage/st_if.c storage/book.c
#TRNG
TRNFILES = storage/rndlib/chacha20.c storage/rndlib/curve25519donna.c storage/rndlib/havege.c storage/rndlib/timing.c storage/rndlib/trng.c
#cryptor
CRPFILES = cryptor/ike.c cryptor/pke.c cryptor/ser.c cryptor/cr.c cryptor/cr_if.c cryptor/ser/devtty.c cryptor/ser/comport.c
#encrypting
CRFILES = cryptor/crplib/KeccakP800.c cryptor/crplib/shake.c
#ellipic curve X25519
ECCFILES = ecc/add.c ecc/copy.c ecc/cswap.c ecc/from_bytes.c ecc/invert.c ecc/mul.c ecc/mul121665.c  ecc/sqr.c ecc/sub.c ecc/to_bytes.c ecc/scalarmult.c ecc/r2p.c 
#transport
TRFILES = transport/jit.c transport/local_if.c transport/tr.c transport/tr_if.c transport/file.c transport/file_if.c
#obfuscator
OBFFILES = transport/obflib/serpent.c transport/obflib/ocb.c transport/obflib/rnd.c
#audio
AUFILES = snd/dev/audio_wave.c snd/dev/audio_alsa.c snd/dev/aecm.c snd/audio.c
#noise supressor NPP7
NPPFILES = snd/npp/dsp_sub.c snd/npp/fft_lib.c snd/npp/fs_lib.c snd/npp/global.c snd/npp/mat_lib.c snd/npp/math_lib.c snd/npp/mathdp31.c snd/npp/mathhalf.c snd/npp/npp.c
#AMR audio codec
AMRFILES = snd/amr/amr_speech_importance.c snd/amr/sp_dec.c snd/amr/sp_enc.c snd/amr/interf_dec.c snd/amr/interf_enc.c

#static library source
LSRC = $(USRFILES) $(AUFILES) $(TRFILES) $(OBFFILES) $(STFILES) $(TRNFILES) $(CRPFILES) $(CRFILES) $(ECCFILES) $(NPPFILES) $(AMRFILES)
#static library objects
LOBJECTS = $(LSRC:.c=.o)



#build the binary.
all: tf libtf

tf: $(USRFILES) $(AUFILES) $(TRFILES) $(OBFFILES) $(STFILES) $(TRNFILES) $(CRPFILES) $(CRFILES) $(ECCFILES) $(NPPFILES) $(AMRFILES)
	$(CC) $(TFPATH) -o $@ $@.c $(USRFILES) $(AUFILES) $(TRFILES) $(OBFFILES) $(STFILES) $(TRNFILES) $(CRPFILES) $(CRFILES) $(ECCFILES) $(NPPFILES) $(AMRFILES) $(TFLIBS)

libtf: $(LOBJECTS)
	rm -f libtf.a
	ar cr libtf.a $(LOBJECTS)
	-if test -s /bin/ranlib; then /bin/ranlib libtf.a; \
      else if test -s /usr/bin/ranlib; then /usr/bin/ranlib libtf.a; \
	else exit 0; fi; fi	
	
clean:
	rm -f *.o *.exe *.a tf
	rm -- **/*.o
	rm -- **/**/*.o
	
	
