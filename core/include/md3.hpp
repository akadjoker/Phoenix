

#pragma once

#define kLower 0  // This stores the ID for the legs model
#define kUpper 1  // This stores the ID for the torso model
#define kHead 2	  // This stores the ID for the head model
#define kWeapon 3 // This stores the ID for the weapon model

typedef enum
{
	// If one model is set to one of the BOTH_* animations, the other one should be too,
	// otherwise it looks really bad and confusing.

	BOTH_DEATH1 = 0, // The first twirling death animation
	BOTH_DEAD1,		 // The end of the first twirling death animation
	BOTH_DEATH2,	 // The second twirling death animation
	BOTH_DEAD2,		 // The end of the second twirling death animation
	BOTH_DEATH3,	 // The back flip death animation
	BOTH_DEAD3,		 // The end of the back flip death animation

	// The next block is the animations that the upper body performs

	TORSO_GESTURE, // The torso's gesturing animation

	TORSO_ATTACK,  // The torso's attack1 animation
	TORSO_ATTACK2, // The torso's attack2 animation

	TORSO_DROP,	 // The torso's weapon drop animation
	TORSO_RAISE, // The torso's weapon pickup animation

	TORSO_STAND,  // The torso's idle stand animation
	TORSO_STAND2, // The torso's idle stand2 animation

	// The final block is the animations that the legs perform

	LEGS_WALKCR, // The legs's crouching walk animation
	LEGS_WALK,	 // The legs's walk animation
	LEGS_RUN,	 // The legs's run animation
	LEGS_BACK,	 // The legs's running backwards animation
	LEGS_SWIM,	 // The legs's swimming animation

	LEGS_JUMP, // The legs's jumping animation
	LEGS_LAND, // The legs's landing animation

	LEGS_JUMPB, // The legs's jumping back animation
	LEGS_LANDB, // The legs's landing back animation

	LEGS_IDLE,	 // The legs's idle stand animation
	LEGS_IDLECR, // The legs's idle crouching animation

	LEGS_TURN, // The legs's turn animation

	MAX_ANIMATIONS // The define for the maximum amount of animations
} eAnimations;

struct tMd3Header
{
	char fileID[4];	  // This stores the file ID - Must be "IDP3"
	int version;	  // This stores the file version - Must be 15
	char strFile[68]; // This stores the name of the file
	int numFrames;	  // This stores the number of animation frames
	int numTags;	  // This stores the tag count
	int numMeshes;	  // This stores the number of sub-objects in the mesh
	int numMaxSkins;  // This stores the number of skins for the mesh
	int headerSize;	  // This stores the mesh header size
	int tagStart;	  // This stores the offset into the file for tags
	int tagEnd;		  // This stores the end offset into the file for tags
	int fileSize;	  // This stores the file size
};

struct tMd3MeshInfo
{
	char meshID[4];	   // This stores the mesh ID (We don't care)
	char strName[68];  // This stores the mesh name (We do care)
	int numMeshFrames; // This stores the mesh aniamtion frame count
	int numSkins;	   // This stores the mesh skin count
	int numVertices;   // This stores the mesh vertex count
	int numTriangles;  // This stores the mesh face count
	int triStart;	   // This stores the starting offset for the triangles
	int headerSize;	   // This stores the header size for the mesh
	int uvStart;	   // This stores the starting offset for the UV coordinates
	int vertexStart;   // This stores the starting offset for the vertex indices
	int meshSize;	   // This stores the total mesh size
};

 

// This stores the bone information (useless as far as I can see...)
struct tMd3Bone
{
	float mins[3];	   // This is the min (x, y, z) value for the bone
	float maxs[3];	   // This is the max (x, y, z) value for the bone
	float position[3]; // This supposedly stores the bone position???
	float scale;	   // This stores the scale of the bone
	char creator[16];  // The modeler used to create the model (I.E. "3DS Max")
};

// This stores the normals and vertex indices
struct tMd3Triangle
{
	signed short vertex[3];	 // The vertex for this face (scale down by 64.0f)
	unsigned char normal[2]; // This stores some crazy normal values (not sure...)
};

// This stores the indices into the vertex and texture coordinate arrays
struct tMd3Face
{
	int vertexIndices[3];
};

// This stores UV coordinates
struct tMd3TexCoord
{
	float textureCoord[2];
};

// This stores a skin name (We don't use this, just the name of the model to get the texture)
struct tMd3Skin
{
	char strName[68];
};
