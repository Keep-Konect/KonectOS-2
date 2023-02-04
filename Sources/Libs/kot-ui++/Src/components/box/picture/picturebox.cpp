#include <kot-ui++/component.h>

#include <kot-ui++/pictures/picture.h>

#include <kot/uisd/srvs/storage.h>

namespace Ui {

    Component* Picturebox(char* path, ImageType type, ImageStyle style) {
        file_t* imageFile = fopen(path, "rb");

        if(imageFile == NULL)
            return NULL;

        fseek(imageFile, 0, SEEK_END);
        size_t imageFileSize = ftell(imageFile);
        fseek(imageFile, 0, SEEK_SET);

        TGAHeader_t* image = (TGAHeader_t*) malloc(imageFileSize);
        fread(image, imageFileSize, 1, imageFile);

        uint16_t Width = image->Width;
        uint16_t Height = image->Height;

        if(Width <= 0 || Height <= 0) { free(image); fclose(imageFile); return NULL; }

        if(style.Width == NULL)
            style.Width = Width;
        else
            Width = style.Width;
        
        if(style.Height == NULL)
            style.Height = Height;
        else
            Height = style.Height; 

        Component* picture = new Component({ .Width = style.Width, .Height = style.Height });

        switch(type)
        {
            case ImageType::_TGA:
                uint32_t* Pixels = TGARead(image, Width, Height);

                for(uint16_t y = 0; y < Height; y++) {
                    for(uint16_t x = 0; x < Width; x++) {
                        PutPixel(picture->GetFramebuffer(), picture->GetStyle()->x+x, picture->GetStyle()->y+y, Pixels[x + y*Width]);
                    }
                }

                free(Pixels);

                break;
        }

        free(image);
        fclose(imageFile);
        
        return picture;
    }

}