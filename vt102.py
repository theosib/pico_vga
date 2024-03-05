def main():
    # Open the output file in write mode
    with open("vt102_test.txt", "w") as file:
        # Clear the screen and set background to blue
        file.write("\033[2J")  # Clear screen
        file.write("\033[H")
        file.write("\033[44m")  # Set background color to blue
        file.write("\033[37m")  # Set foreground color to white
        
        # Write text with various attributes
        file.write("This is a test of VT102 terminal emulator.\n")
        file.write("\033[1mBold Text\n")  # Bold
        file.write("\033[3mItalic Text\n")  # Italic
        file.write("\033[4mUnderlined Text\n")  # Underline
        file.write("\033[7mInverse Text\n")  # Inverse (background/foreground swap)
        
        # Create "window" areas with different background colors
        file.write("\033[0m")  # Reset attributes
        file.write("\n\n")  # Add some space
        
        file.write("\033[41m")  # Set background color to red
        file.write("Window 1\n\n")
        
        file.write("\033[42m")  # Set background color to green
        file.write("Window 2\n\n")
        
        file.write("\033[43m")  # Set background color to yellow
        file.write("Window 3\n\n")
        
        file.write("\033[0m")  # Reset attributes
        
        print("VT102 test file generated successfully.")

if __name__ == "__main__":
    main()
