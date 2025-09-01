plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.op.opluskey"
    compileSdk {
        version = release(36)
    }

    defaultConfig {
        applicationId = "com.op.opluskey"
        minSdk = 30
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
        ndk {
            //noinspection ChromeOsAbiSupport
            abiFilters += "arm64-v8a"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

dependencies {
}

// 任务 1: 复制二进制文件到模板目录（保持不变）
tasks.register("copyBinariesToTemplate") {
    description = "构建模块时，将编译好的 C++ 二进制文件复制到 Magisk 模板目录"
    group = "MagiskBuild"
    dependsOn("externalNativeBuildRelease")
    doLast {
        println("--- 开始执行 copyBinariesToTemplate ---")
        val abi = "arm64-v8a"
        val nativeBuildDir = file("$buildDir/intermediates/cxx/RelWithDebInfo")
        println(">> [调试] 1. 基础搜索目录路径: ${nativeBuildDir.absolutePath}")
        println(">> [调试] 2. 基础目录是否存在? ${nativeBuildDir.exists()}")
        if (!nativeBuildDir.exists()) {
            throw GradleException("错误：基础搜索目录不存在，中止执行。")
        }
        val objDir = nativeBuildDir.walk().firstOrNull { it.isDirectory && it.name == "obj" }
        if (objDir != null) {
            println(">> [调试] 3. 已找到 'obj' 目录于: ${objDir.absolutePath}")
        } else {
            throw GradleException("错误：在基础路径中寻找 'obj' 目录失败: $nativeBuildDir")
        }
        val abiBinDir = file("${objDir.path}/$abi")
        println(">> [调试] 4. 预期的二进制文件 ABI 目录: ${abiBinDir.absolutePath}")
        println(">> [调试] 5. ABI 目录是否存在? ${abiBinDir.exists()}")
        if (abiBinDir.exists()) {
            val filesInDir = abiBinDir.listFiles()?.joinToString(", ") { it.name } ?: "此目录为空或不是一个有效目录"
            println(">> [调试] 6. ABI 目录中找到的文件: [${filesInDir}]")
            project.copy {
                from(abiBinDir)
                into(file("src/magisk_template"))
            }
            println(">> 成功: 复制操作已执行。")
        } else {
            throw GradleException("错误：ABI 目录未找到: $abiBinDir")
        }
        println("--- copyBinariesToTemplate 执行完毕 ---")
    }
}

// 任务 2: 【新增】将模板文件夹打包成 ZIP
tasks.register<Zip>("packageMagiskModuleZip") {
    description = "将包含最新二进制文件的模板文件夹打包成可刷入的 ZIP 包"
    group = "MagiskBuild"

    // 关键：确保此任务在复制任务完成后再执行
    dependsOn(tasks.named("copyBinariesToTemplate"))

    // 设置要打包的源文件夹
    from(file("src/magisk_template"))
    // 设置 zip 包内部的根目录（可选，这里设置为空，即直接打包文件夹内容）
    archiveBaseName.set("OplusKey_Magisk") // 输出的 zip 文件名
    archiveVersion.set("1.0.3") // 版本号

    // 【关键】设置输出路径为你指定的目录
    destinationDirectory.set(file("$rootDir/mod"))
}

// 任务 3: 【更新】将最终的打包任务链接到 Android 的主构建流程
afterEvaluate {
    tasks.named("assembleRelease").configure {
        // 让 assembleRelease 任务依赖我们最终的打包任务
        dependsOn(tasks.named("packageMagiskModuleZip"))
    }
}