torfone
=======

TORFone - voice add-on for TorChat: voice-over-Tor and p2p secure and anonymous VoIP tool

TORFone V1.1b (01.06.13) features:

    * useable with TorChat as a voice add-on;
    * full duplex, PTT and VOX modes;
    * fully portable (can be run from removable disk or TrueCrypt volume);
    * can be run under virtual Win32 OS or Wine under any linux;
    * own protected chat;
    * protected files transfer;
    * fully open source (compiled by VC6 under WinXP without any external libs)

Cryptography:

    * Diffie-Hellman 4096 under Tor;
    * AES 256 OCB mode (with 128 bit MAC);
    * PKDF2/HMAC passphrase autentification;
    * anti-MitM and silent notification under pressure

Voice processing:

    * MELPE, CODEC2, OPUS (with hiding vbr), GSM, ADCPM codecs;
    * noise supressor NPP7;
    * build-in security vocoder;
    * jitter compensation specially adapted for Tor

Transport:

    * extremally low bandwidth (from 250 bytes/s);
    * Tor HS multi-connection for decreasing latency;
    * Tor like a SIP (feature for NAT traversal and followed HQ direct UDP connection);
    * direct TCP or UDP connection using a custom protocol (some DPI protection)

