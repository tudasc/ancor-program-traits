
#include <stdio.h>
#include "../library.h"

int main(int argc, char** argv){
    struct trait_options options;
    options.name="sample";
    trait_handle_type trait = register_trait(&options);

    bool result = check_trait(trait);
    printf("Hello World\n");
    printf("Trait is %s\n",result ? "true" : "false" );

    remove_trait(trait);
}