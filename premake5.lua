workspace("mazelock")
configurations({ "release", "debug" })

newoption({
    trigger = "usepng",
    description = "Load textures from PNG files instead of procedural generation"
})

project("mazelock")
kind("ConsoleApp")
language("C++")
cppdialect("C++23")
targetdir("bin")
files({ "src/**.cpp" })
includedirs({ "src", "vendor", "/usr/include/libdrm", "/usr/include/drm" })

filter("configurations:release")
optimize("Speed")
defines({ "NDEBUG" })

filter("system:linux")
if _OPTIONS["usepng"] then
    defines({ "USEPNG" })
    links({ "drm", "pthread", "X11", "Xinerama", "png" })
else
    links({ "drm", "pthread", "X11", "Xinerama" })
end
