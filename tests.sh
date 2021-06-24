#!/bin/bash

echo "Número da máscara:"
echo $1

echo 1
time ./filter 1 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 2
time ./filter 2 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 3
time ./filter 3 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 4
time ./filter 4 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 5
time ./filter 5 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 6
time ./filter 6 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 7
time ./filter 7 $1 ./images/samples/buracoNegro.bmp
echo ""
echo 8
time ./filter 8 $1 ./images/samples/buracoNegro.bmp
