val base = env.library("base")
val table = env.library("table")
val vector = env.library("vector")
val string = env.library("string")
val value = env.library("value")
val sio = env.library("sio")
val gc = env.library("gc")
val fs = env.library("fs")
val compiler = env.library("compiler")
val binary = env.library("binary")

fun help<auto>(program, message, usage)
    base.println("Morphine (version: " .. base.version().. ")")

    if (message != nil)
        sio.write(sio.error(), "Message: " .. message .. "\n")
    end

    if (usage)
        base.println("Usage: " .. program .. " [options] [program [args]]")
        base.println("Positional arguments:")
        base.println("    program         Path to program")
        base.println("    args            Program arguments")
        base.println("Optional arguments:")
        base.println("    -b              Binary program")
        base.println("    -r              Run program")
        base.println("    -e path         Export program")
        base.println("    -n name  [main] Program name (gives an effect for a non-binary format)")
        base.println("    -d              Disassembly program")
        base.println("    -m bytes [256M] Allocation limit (possible suffixes: K, M, G)")
        base.println("    -h              Print help message")
        base.println("    --              Stop handling options")
    end

    if (message != nil)
        env.exit(1)
    end
end

fun tobytes<auto>(program, param)
    if (lenof param < 1)
        help(program, "invalid bytes param", true)
    end

    fun convert<auto>()
        val decompose modifier, param = do
            val last = string.charat(param, lenof param - 1)

            val int = if (string.isalpha(last))
                value.toint(string.substring(param, 0, lenof param - 1))
            else
                value.toint(param)
            end

            val modifier = if (string.isalpha(last))
                string.tolower(last)
            else
                nil
            end

            { modifier = modifier, param = int }
        end

        return if (modifier == nil)
            param
        elif (modifier == 'k')
            param * 1024
        elif (modifier == 'm')
            param * 1024 * 1024
        elif (modifier == 'g')
            param * 1024 * 1024 * 1024
        else
            nil
        end
    end

    val decompose result, error = base.pcall(convert)

    if (result == nil)
        help(program, "invalid bytes param", true)
    end

    return result
end

fun parse_arguments<auto>()
    if (lenof env.args < 1)
        help("morphine", "empty command line arguments", true)
    end

    if (lenof env.args == 1)
        help("morphine", "no arguments", true)
    end

    val result = table.fixed {
        app_path = env.args[0],
        program_path = nil,
        args = 0,
        gc_limit = 256 * 1024 * 1024,
        binary = false,
        execute = false,
        name = "main",
        disassembly = false,
        export = nil,
        help = false
    }

    var require_path = false
    var parsed = 1
    for(pass; parsed < lenof env.args; parsed ++)
        val argument = env.args[parsed]
        if (lenof argument == 0)
            help(result.app_path, "empty command line arguments", true)
        end

        val chars = string.chars(argument)

        if (chars[0] != '-')
            break
        end

        var offset = 0
        var stop = false
        for (var i = 1; i < lenof chars; i ++)
            val char = chars[i]
            if (char == '-')
                stop = true
                break
            elif (char == 'b')
                require_path = true
                result.binary = true
            elif (char == 'r')
                require_path = true
                result.execute = true
            elif (char == 'd')
                require_path = true
                result.disassembly = true
            elif (char == 'e')
                require_path = true
                offset ++
                if (parsed + offset >= lenof env.args)
                    help(result.app_path, "no export path", true)
                end
                result.export = env.args[parsed + offset]
            elif (char == 'n')
                require_path = true
                offset ++
                if (parsed + offset >= lenof env.args)
                    help(result.app_path, "no name", true)
                end
                result.name = env.args[parsed + offset]
            elif (char == 'm')
                offset ++
                if (parsed + offset >= lenof env.args)
                    help(result.app_path, "no bytes value", true)
                end
                val value = env.args[parsed + offset]
                result.gc_limit = tobytes(result.app_path, value)
            elif (char == 'h')
                result.help = true
            else
                help(result.app_path, nil, true)
            end
        end

        parsed += offset

        if (stop)
            break
        end
    end

    if (parsed >= lenof env.args)
        if (require_path)
            help(result.app_path, "program path is expected", true)
        else
            return result
        end
    end

    result.program_path = env.args[parsed]
    result.args = parsed + 1

    return result
end

fun launch<auto>()
    val decompose config as "result" = base.pcall(parse_arguments)

    if (config == nil)
        leave
    end

    if (config.help)
        help(config.app_path, nil, true)
    end

    gc.changelimit(config.gc_limit)

    if (config.program_path != nil)
        val content = if (config.binary)
            val file = fs.file(config.program_path, "rb")
            val result = binary.receive(file)
            sio.close(file)

            result
        else
            val file = fs.file(config.program_path, "rb")
            val text = sio.extractstring(file)
            sio.close(file)

            fun compile<auto>() = compiler.compile {
                text = text,
                name = config.name
            }

            val decompose result, error = base.pcall(compile)
            if (error != nil)
                help(config.app_path, error, false)
            end

            result
        end

        if (config.disassembly)
            compiler.disassembly(sio.io(), content)
            base.println()
        end

        if (config.export != nil)
            val file = fs.file(config.export, "wb")
            binary.send(file, content)
            sio.close(file)
        end

        if (config.execute)
            val environment = {
                library = env.library,
                args = vector.unfixed([])
            }

            for(var i = config.args; i < lenof env.args; i ++)
                vector.push(environment.args, env.args[i])
            end

            base.changeenv(environment, content)
        end
    end
end

launch()