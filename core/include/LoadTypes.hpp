#pragma once
#include "Config.hpp"



// Chunk IDs do formato 3DS
enum Chunk3DS
{
    // Principais
    MAIN3DS         = 0x4D4D,
    EDIT3DS         = 0x3D3D,
    KEYF3DS         = 0xB000,
    
    // Material
    EDIT_MATERIAL   = 0xAFFF,
    MAT_NAME        = 0xA000,
    MAT_AMBIENT     = 0xA010,
    MAT_DIFFUSE     = 0xA020,
    MAT_SPECULAR    = 0xA030,
    MAT_TEXMAP      = 0xA200,
    MAT_MAPNAME     = 0xA300,
    
    // Objeto
    EDIT_OBJECT     = 0x4000,
    OBJ_TRIMESH     = 0x4100,
    OBJ_LIGHT       = 0x4600,
    OBJ_CAMERA      = 0x4700,
    
    // Mesh
    TRI_VERTEXL     = 0x4110,
    TRI_FACEL1      = 0x4120,
    TRI_MAPPINGCOORS= 0x4140,
    TRI_MATERIAL    = 0x4130,
    TRI_SMOOTH      = 0x4150,
    TRI_LOCAL       = 0x4160,
    
    // Color
    COLOR_F         = 0x0010,
    COLOR_24        = 0x0011,
    LIN_COLOR_24    = 0x0012,
    LIN_COLOR_F     = 0x0013,
    
    // Percentagem
    INT_PERCENTAGE  = 0x0030,
    FLOAT_PERCENTAGE= 0x0031
};

 