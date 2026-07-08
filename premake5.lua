workspace("mazelock")
configurations({ "release", "debug" })

newoption({
	trigger = "usepng",
	description = "Load textures from PNG files instead of procedural generation",
})

project("mazelock")
kind("ConsoleApp")
language("C++")
cppdialect("C++23")
targetdir("bin")
files({ "src/**.cpp" })
includedirs({ "src", "vendor", "/usr/include/libdrm", "/usr/include/drm" })
defines({ [[PASSWORD="world90s"]] })

filter("configurations:release")
optimize("Speed")
buildoptions({ "-march=native" })
buildoptions({ "-flto" })
linkoptions({ "-flto" })
defines({ "NDEBUG" })

filter("configurations:debug")
optimize("Debug")
linkoptions({ "-Wl,-Map=bin/mazelock.map" })
symbols("On")

filter("system:linux")
if _OPTIONS["usepng"] then
	defines({ "USEPNG" })
	links({ "drm", "pthread", "X11", "Xext", "Xinerama", "png" })
else
	links({ "drm", "pthread", "X11", "Xext", "Xinerama" })
end
