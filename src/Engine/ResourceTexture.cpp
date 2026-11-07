#include "ResourceTexture.h"
#include <glad/glad.h>

ResourceTexture::~ResourceTexture() {
    if (id != 0) glDeleteTextures(1, &id);
}