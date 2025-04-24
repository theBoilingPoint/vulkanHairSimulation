#pragma once

typedef unsigned char stbi_uc;

enum Shader {
	VERTEX,
	FRAGMENT
};

struct Image {
	int width;
	int	height;
	int	channels;
	stbi_uc* pixels;
};