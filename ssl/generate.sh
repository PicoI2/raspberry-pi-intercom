#!/bin/bash
openssl req -x509 -newkey rsa:4096 -keyout privkey1.pem -out cert1.pem -days 365
