
#include <stdio.h>
#include "../library.h"

#define EXAMPLE_PROPERTY sample
marker(sample)

int this_is_another_sample;

int main(int argc, char** argv){
    struct trait_options options;
    options.name="sample";
    trait_handle_type trait = register_trait(&options);

    bool result = check_trait(trait);
    printf("Hello World\n");
    printf("Trait is %s\n",result ? "true" : "false" );

    remove_trait(trait);
}
