#include <iostream>
#include <fstream>
#include <cassert>

//
// extracts the file name (with extension) from a path
// TODO: handle scaped '/' characters (e.g.: "/home/user/my\/file\/name.png" -> "my\/file\/name.png")
//
std::string filename(const std::string& path)
{
  // find the last occurrence of the path separator.
  //
  // TODO: here we assume it is the last occurrence of the character '/',
  // but in fact it should be the last `unescaped` occurrence (i.e. without a '\' preceding it).
  auto p = path.find_last_of('/');

  // if there is no path separator (e.g. when the path is relative and the file is in the working directory)
  // the path coincides with the file name, otherwise, it is a substring of it
  std::string file = (p == std::string::npos) ? path : path.substr(p+1);

  // we will want to encode the size of this file name as an unsigned 8-bit intenger type, so we need to make sure
  // it will fit
  if (file.size() > 255)
  {
    std::cerr << "Warning: file name \"" << file << "\" is more than 256 characters long. It was truncated to \"";
    file = file.substr(file.size()-255);
    std::cerr << file << "\"." << std::endl;
  }

  return file;
}

//
// finds the number of bytes between the current position and the end of the stream
// TODO: we might want to rename this function to better reflect what it does
//
uint32_t stream_size(std::ifstream& s)
{
  auto from = s.tellg();
  s.seekg(0, std::ios::end);
  auto to = s.tellg();
  auto size = to-from;
  s.seekg(from, std::ios::beg);

  // we will want to encode the size as an unsigned 32-bit intenger, so we need to make sure
  // it will fit
  if (size < static_cast<std::streamsize>(size))
  {
    std::cerr << "Error: Computed a negative value for the png file size. Something was very wrong with the input stream!" << std::endl;
    exit(EXIT_FAILURE);
  }
  if (size > std::numeric_limits<uint32_t>::max())
  {
    std::cerr << "Error: The png file size is too big to fit in an unsigned 32-bit number." << std::endl;
    exit(EXIT_FAILURE);
  }
  return static_cast<uint32_t>(size);
}

//
// writes an uint32_t to a binary stream in big-endian byte order
//
void writeBigEndian(std::ostream& out, uint32_t n)
{
  char be[4];
  for (int i=0; i < 4; ++i) {
    be[i] = (n >> ((3-i)<<3)) & 0xFF;
  }
  out.write(&be[0], 4);
}

//
// program usage information string
// @arg prg the name of the program (usually std::string(argv[0]))
// @ret a string representing the usage information
//
std::string usage(const std::string& prg)
{
  return prg + " [png_path stl_path output_path]";
}


int main(int argc, char* argv[])
{
  // check command-line input
  if (argc < 1) {
    std::cerr << "The program name was not passed as a command line argument. There is a problem with the system." << std::endl;
    exit(EXIT_FAILURE);
  }
  if (argc != 4) {
    std::cout << usage(std::string{argv[0]}) << std::endl;
    exit(EXIT_FAILURE);
  }

  // file names
  std::string png_file{argv[1]};
  std::string stl_file{argv[2]};
  std::string stl2png_file{argv[3]};

  // I/O file streams
  std::ifstream png{png_file, std::ios::binary};
  if (!png.is_open())
  {
    std::cerr << "Failed to open file \"" << png_file << "\" for reading." << std::endl;
    exit(EXIT_FAILURE);
  }
  std::ifstream stl{stl_file, std::ios::binary};
  if (!stl.is_open())
  {
    std::cerr << "Failed to open file \"" << stl_file << "\" for reading." << std::endl;
    exit(EXIT_FAILURE);
  }
  std::ofstream stl2png{stl2png_file, std::ios::binary};
  if (!stl2png.is_open())
  {
    std::cerr << "Failed to open file \"" << stl2png_file << "\" for writing." << std::endl;
    exit(EXIT_FAILURE);
  }

  // base names of the input files
  png_file = filename(png_file);
  stl_file = filename(stl_file);

  assert(png_file.size() < 256u);
  assert(stl_file.size() < 256u);

  // content size of the png file
  uint32_t png_size = stream_size(png);

  // the output file structure
  stl2png << png.rdbuf() << stl.rdbuf() << png_file << stl_file;
  stl2png << static_cast<uint8_t>(png_file.size()) << static_cast<uint8_t>(stl_file.size());
  writeBigEndian(stl2png, png_size);

  return 0;
}

