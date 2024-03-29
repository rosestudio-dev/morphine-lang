buildscript {
    repositories {
        gradlePluginPortal()
        google()
        mavenCentral()
    }

    dependencies {
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:1.9.23")
        classpath("com.google.devtools.ksp:com.google.devtools.ksp.gradle.plugin:1.9.23-1.0.19")
    }
}

group = "ru.unit.morphine"

subprojects {
    val monoBuildDir = System.getProperty("monoBuildDir")?.let { monoBuildDir ->
        File(
            File(monoBuildDir).apply { mkdir() },
            project.fullname
        )
    }

    val projectVersion = System.getProperty("projectVersion") ?: "unknown"

    if (monoBuildDir != null) {
        layout.buildDirectory.set(monoBuildDir)
    }

    version = projectVersion
}

val Project.fullname
    get() = run {
        val projects = mutableListOf<Project>()

        var current: Project? = this
        while (current != null) {
            projects.add(current)
            current = current.parent
        }

        projects.reversed().joinToString(separator = "_", transform = Project::getName)
    }
