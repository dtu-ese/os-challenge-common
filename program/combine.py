import os
import re


def combine_c_and_headers(c_program, output_file):
    # Get the current working directory (same folder as the Python script)
    current_directory = os.getcwd()

    # Ensure the specified C program exists
    if not os.path.exists(os.path.join(current_directory, c_program)):
        print(f"{c_program} not found in the directory.")
        return

    # Open the output file in write mode
    with open(output_file, "w") as outfile:
        # Write the C program content
        outfile.write(f"===== {c_program} =====\n")
        with open(os.path.join(current_directory, c_program), "r") as infile:
            c_code = infile.read()
            outfile.write(c_code)
            outfile.write("\n\n")

        # Find headers included in the C program
        headers = re.findall(r'#include\s+[<"](.+\.h)[">]', c_code)

        # Process each header file included in the C program
        for header in headers:
            header_path = os.path.join(current_directory, header)
            if os.path.exists(header_path):
                outfile.write(f"===== {header} =====\n")
                with open(header_path, "r") as hfile:
                    outfile.write(hfile.read())
                outfile.write("\n\n")
            else:
                outfile.write(f"// Header {header} not found in the directory.\n\n")

    print(f"{c_program} and its headers have been combined into {output_file}")


# Example usage:
# Replace 'your_c_program.c' with the C program file in your directory.
combine_c_and_headers("server.c", "combined_program.txt")
