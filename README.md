Torfone
=======

Torfone is an utility that works under Tor transporting compressed voice packets. Effective voice queue and use of duplicate circuits significantly reduces speech latency now is comparable to satellite phones. In addition users who do not care about anonymity can switch to direct connection mode by performing a NAT traversal using the established Tor connection. This functionality provides unique server-less VOIP solution works without any online registration and prevents the collection of metadata.

Torfone provides its own level of obfuscation, encryption and authentication using modern cryptography.  Our protocol is not standard so you can ignore its presence fully trusting Tor. Nevertheless it provides reliable protection on primary direct connecting mode using the IP address and port as the destination of subscriber:

-	Connecting is performed in two physical TCP sessions with random time interval (for some DPI protecting).
-	Exchange Elligator2 key representations with random padding during first session (against censorship).
-	Authenticate encrypting and random padding for all transcripts in second session with key derived in first session (for some DPI protecting)
-	Proof of job required in second session (for protecting against scanners). 
-	Encrypting and decrypting symmetric keys derives with only DH and not depends from authentication (allowed unauthenticated sessions).
-	DH secret is unpredictable due commitment (allowed comparing short secret’s fingerprints: SAS) 
-	Zero knowledge mutual authentication by application public keys (Triple DH extended with SPEKE secret for zero knowledge property)
-	KCI resistance (for leak of long term public keys)
-	Deniability for using private key (in the case of other party communicate with Judge after session)
-	Forward deniability (in the case of other party communicate with Judge before session to known participant)
-	Protecting of originator’s ID against passive and active attacks (interrupted probes and interceptions) with PFS.
-	Automatic receiving any keys send by other party saved with generated names (for deniability of having the key).
-	Zero knowledge comparing of pre-shared secrets during unauthenticated session (like SMP in OTR but uses SPEKE)
Authentication of caller’s onion address based on Hidden Service security (as in TorChat)  

Alpha version for Android is now available for testing allowing to estimate the latency of calls in the Tor network:
http://torfone.org/download/Torfone.apk

http://torfone.org/download/Torfone_Android_howto.pdf

Binary files with GUI for Linux (386 and ARM) and Windows are also available on request by email as well as their source codes in the form of projects in the corresponding development environments. Later all source codes and binary files will be available via the links on the project page (now in development): 
http://torfone.org

Some technical information about the structure of the library and cryptographic protocols is given in the alpha draft: white.pdf

Van Gegel,  torfone@ukr.net

