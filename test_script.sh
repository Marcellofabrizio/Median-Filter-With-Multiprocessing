#!/bin/bash

# $1 = executável
# $2 = caminho da imagem
# $3 = máscara


echo "#### INICIANDO TESTE COM MÁSCARA $3 X $3 ####"
for (( i=1; i <=8; i++ ))
do
  echo "--> $i INSTÂNCIA(S)"
  time $1 $i $3 $2
  echo
  echo "#############################################"
done
echo "######## MOSTRANDO IMAGEM PROCESSADA ########"
xdg-open /home/marcello/Documents/university/operating-systems/bmp-median-filter/images/results/correctedImage.bmp
