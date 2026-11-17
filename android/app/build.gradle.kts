plugins {
    alias(libs.plugins.android.application)
}

android {
    ndkVersion = "30.0.16248370"    // Just using the newest one on my computer IDK
    namespace = "com.example.petrenderer"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.example.petrenderer"
        minSdk = 31
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // limit which abi's get built by gradle (application binary interfaces):
        // https://developer.android.com/ndk/guides/abis.html#sa
        // if you need to build for a different abi then you would need to add it here...
        ndk { abiFilters += "arm64-v8a"}

        // https://stackoverflow.com/questions/75870769/why-am-i-getting-java-lang-unsatisfiedlinkerror-dlopen-failed-library-libc
        // idk man the layer library needs the cpp standard library or something
        // https://developer.android.com/ndk/guides/graphics/validation-layer#vl-external
        // also apparently we don't even need to do this so maybe in the future I take that .so file
        // out and like turn it off
        externalNativeBuild {
            cmake {
                arguments += "-DANDROID_STL=c++_shared"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "4.1.2"
        }
    }

    sourceSets["main"].assets.directories.add("../../assets")

    buildTypes {
        release {
            optimization {
                enable = false
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.core.ktx)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(libs.androidx.junit)
}