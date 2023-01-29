#include "layout.h"

using namespace Ui;

namespace UiLayout {

    void calculateFlex(Component* parent, uint32_t index) {

        Component* child = (Component*) vector_get(parent->getChilds(), index);

        // todo: fill/normal mode
        
        switch (parent->getStyle()->direction)
        {
            case Layout::HORIZONTAL:
            {
                // todo: parent -> Width: 100%

                if(parent->getStyle()->Height < child->getStyle()->Height)
                    parent->getStyle()->Height = child->getStyle()->Height;

                /* space */
                if(parent->getStyle()->space == Layout::BETWEEN) {
                    
                    

                } // else around...

                break;
            }

            case Layout::VERTICAL:
            {
                

                break;
            }
        }

        // if the child is the last
        if(index+1 == parent->getChilds()->length)
            recalculateFlexChilds(parent);

    }

    void recalculateFlexChilds(Component* parent) {

        for(int i = 0; i < parent->getChilds()->length; i++) {

            Component* child = (Component*) vector_get(parent->getChilds(), i);

            child->getStyle()->Height = parent->getStyle()->Height;

        }

    }

}