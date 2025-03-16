#include <stdio.h>
#include <stdlib.h>
#include <hw/inout.h>
#include <sys/mman.h> 
#include <stdint.h>

// Define GPIO base address for Raspberry Pi
#define BCM2837_GPIO_BASE 0x3F200000
#define GPIO_LEN 0xB0

// Define GPIO pin numbers for joystick (adjust based on your wiring)
#define JOYSTICK_X_PIN 17
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_SW_PIN 22

// Global variable for GPIO memory map
volatile uint32_t* gpio_map;

// Function to set up GPIO pin as input
void setup_gpio_input(int pin) {
    int reg = pin / 10;
    int shift = (pin % 10) * 3;
    *(gpio_map + reg) &= ~(7 << shift);  // Clear bits
    *(gpio_map + reg) |=  (0 << shift);  // Set as input
}

// Function to read GPIO pin
int read_gpio(int pin) {
    return (*(gpio_map + 13) & (1 << pin)) ? 1 : 0;
}

// Function to display the current story and choices
void display_story(const char *story, const char *choices[], int num_choices) {
    printf("\n%s\n", story);
    for (int i = 0; i < num_choices; i++) {
        printf("%d. %s\n", i + 1, choices[i]);
    }
    printf("Use the joystick to select an option and press the button to confirm.\n");
}

int main() {
    // Map GPIO memory using QNX-specific function
    gpio_map = (volatile uint32_t*)mmap_device_memory(
        NULL,             // Any address in our space
        GPIO_LEN,         // Map length
        PROT_READ | PROT_WRITE, // Enable reading & writing
        0,                // Flags
        BCM2837_GPIO_BASE // Physical address of GPIO peripheral
    );

    if (gpio_map == MAP_FAILED) {
        perror("mmap_device_memory error");
        return -1;
    }

    // Set up joystick pins as inputs
    setup_gpio_input(JOYSTICK_X_PIN);
    setup_gpio_input(JOYSTICK_Y_PIN);
    setup_gpio_input(JOYSTICK_SW_PIN);

    // Story and choices
    const char *story = "You are in a dark forest. You hear strange noises around you.";
    const char *choices[] = {
        "Go left towards the sound.",
        "Go right towards the light.",
        "Stay where you are."
    };
    int num_choices = 3;

    int selection = 0; // Current selection (0-based index)
    int confirmed = 0; // Flag to check if selection is confirmed

    while (1) {
        // Display the story and choices
        display_story(story, choices, num_choices);

        // Highlight the current selection
        printf("> %s\n", choices[selection]);

        // Read joystick input
        int x_value = read_gpio(JOYSTICK_X_PIN);
        int y_value = read_gpio(JOYSTICK_Y_PIN);
        int sw_value = read_gpio(JOYSTICK_SW_PIN);

        // Handle joystick input
        if (x_value == 0) {
            printf("Joystick moved LEFT\n");
        } else if (x_value == 1) {
            printf("Joystick moved RIGHT\n");
        }
        
        if (y_value == 0) { // Joystick moved UP
            selection = (selection - 1 + num_choices) % num_choices; // Move selection up
        } else if (y_value == 1) { // Joystick moved DOWN
            selection = (selection + 1) % num_choices; // Move selection down
        }

        if (sw_value == 0) { // Joystick button pressed
            confirmed = 1;
            break; // Exit the loop to proceed with the selected choice
        }

        usleep(200000); // Sleep for 200ms to debounce
    }

    // Handle the selected choice
    switch (selection) {
        case 0:
            printf("\nYou go left towards the sound. You find a hidden treasure!\n");
            break;
        case 1:
            printf("\nYou go right towards the light. It leads you out of the forest.\n");
            break;
        case 2:
            printf("\nYou stay where you are. The noises grow louder, and you feel uneasy.\n");
            break;
        default:
            printf("\nInvalid selection.\n");
            break;
    }

    // Cleanup
    munmap_device_memory((void*)gpio_map, GPIO_LEN);

    return 0;
}