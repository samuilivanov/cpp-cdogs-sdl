-- premake5.lua
workspace "cdogs-cdl"
   configurations { "Debug", "Release" }
   
	buildoptions
	{
		"`pkg-config --cflags --libs gtk+-3.0`",
		"`pkg-config --cflags sdl2`",
		"`pkg-config --cflags --libs SDL2_mixer`",
	}
	linkoptions
	{ 
		"`pkg-config --libs sdl2`",
		"-lSDL2_image",
		"-lSDL2_mixer",
		"-lm"
	}
	
	
project "cdogs-sdl"
	kind "ConsoleApp"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"
	
	files
	{
		"src/**.h",
		"src/**.cpp"
	}
	removefiles
	{
		"src/cdogsed/**",
		"src/tests/**",
	}
	
	includedirs
	{
		"src/",
		"src/cdogs",
		"src/cdogs/include/",
		"src/cdogs/proto/nanopb/",
		"src/cdogs/enet/include/",
		"src/cdogs/yajl/api/"
	}
	
	filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"