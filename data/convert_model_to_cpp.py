#!/usr/bin/env python3
# script pour convertir le modele tflite en tableau c++

import os

# chemin du modele tflite
model_path = 'data/model/stress_detector_model.tflite'
output_path = 'include/model_data.h'

# lire le modele
with open(model_path, 'rb') as f:
    model_data = f.read()

# generer le fichier header
with open(output_path, 'w') as f:
    f.write('#ifndef MODEL_DATA_H\n')
    f.write('#define MODEL_DATA_H\n\n')
    f.write('// tableau contenant le modele tflite\n')
    f.write('const unsigned char model_data[] = {\n')
    
    # ecrire les octets
    for i, byte in enumerate(model_data):
        if i % 12 == 0:
            f.write('    ')
        f.write(f'0x{byte:02x}, ')
        if (i + 1) % 12 == 0 or i == len(model_data) - 1:
            f.write('\n')
    
    f.write('};\n\n')
    f.write(f'const int model_data_size = {len(model_data)};\n\n')
    f.write('#endif // MODEL_DATA_H\n')

print(f'✅ modele converti et sauvegarde dans {output_path}') 