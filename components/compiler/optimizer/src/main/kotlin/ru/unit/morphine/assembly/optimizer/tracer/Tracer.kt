package ru.unit.morphine.assembly.optimizer.tracer

import ru.unit.morphine.assembly.bytecode.AbstractInstruction
import ru.unit.morphine.assembly.bytecode.Argument
import ru.unit.morphine.assembly.bytecode.Instruction
import ru.unit.morphine.assembly.bytecode.Bytecode
import ru.unit.morphine.assembly.bytecode.Value
import ru.unit.morphine.assembly.optimizer.tracer.functions.controlFlow
import ru.unit.morphine.assembly.optimizer.tracer.functions.reconstructFunction
import ru.unit.morphine.assembly.optimizer.tracer.functions.toData
import ru.unit.morphine.assembly.optimizer.tracer.functions.traceDestinations
import ru.unit.morphine.assembly.optimizer.tracer.functions.traceUses
import ru.unit.morphine.assembly.optimizer.tracer.functions.traceValues
import ru.unit.morphine.assembly.optimizer.tracer.functions.traceVersions

class Tracer(
    private val debug: Boolean
) {

    fun trace(function: Bytecode.Function): Data {
        val data = function.toData()

        return if (function.instructions.isEmpty()) {
            data
        } else {
            data.controlFlow(debug).apply {
                traceDestinations()
                traceUses()
                traceVersions()
                traceValues()
            }.data
        }
    }

    fun reconstruct(data: Data) = data.reconstructFunction()

    data class Data(
        val nodes: List<Node>,
        val blocks: List<Block>,
        val function: Bytecode.Function,
    ) {
        var controlFlowTree: ControlFlowTree? = null
    }

    data class Block(
        val id: Int,
        val start: Int,
        val end: Int,
    ) {

        private val range = start until end

        fun nodes(data: Data) = data.nodes.subList(start, end)

        fun edges(data: Data): List<Block> {
            val positions = when (val endNode = data.nodes.getOrNull(end - 1)?.abstract?.instruction) {
                is Instruction.Jump -> listOf(endNode.position.value)
                is Instruction.JumpIf -> listOf(endNode.ifPosition.value, endNode.elsePosition.value)
                is Instruction.Leave -> listOf(data.nodes.size)
                else -> listOf(end + 1)
            }

            return positions.flatMap { pos ->
                data.blocks.filter { b -> pos in b.range }
            }
        }
    }

    data class Node(
        val index: Int,
        val abstract: AbstractInstruction,
        var tracedVersionsBefore: List<TracedVersion>,
        var tracedVersionsAfter: List<TracedVersion>,
        var tracedValuesBefore: List<TracedValue>,
        var tracedValuesAfter: List<TracedValue>,
        var tracedUsesBefore: List<TracedUse>,
        var tracedUsesAfter: List<TracedUse>,
        var destinations: Set<Int>
    ) {
        var isAvailable = true

        fun getSourceVersion(source: Argument.Slot) = tracedVersionsBefore[source.value]
        fun getDestVersion(source: Argument.Slot) = tracedVersionsAfter[source.value]

        fun getSourceValue(source: Argument.Slot) = tracedValuesBefore[source.value]
        fun getDestValue(source: Argument.Slot) = tracedValuesAfter[source.value]

        fun getSourceUse(source: Argument.Slot) = tracedUsesBefore[source.value]
        fun getDestUse(source: Argument.Slot) = tracedUsesAfter[source.value]
    }

    sealed interface TracedVersion {

        data class Normal(val version: Int) : TracedVersion {

            override fun toString() = "$version"
        }

        data class Phi(val versions: Set<Int>) : TracedVersion {

            override fun toString() = "(${versions.joinToString()})"
        }
    }

    sealed interface TracedValue {

        data class Data(
            val index: Int,
        ) : TracedValue {

            override fun toString() = "d$index"

        }

        data class Constant(
            val index: Int
        ) : TracedValue {

            override fun toString() = "c$index"
        }

        data class Phi(val values: Set<TracedValue>) : TracedValue {

            override fun toString() = "(${values.joinToString()})"
        }
    }

    enum class TracedType(val short: String) {
        NIL("N"),
        INTEGER("I"),
        DECIMAL("D"),
        BOOLEAN("B"),
        STRING("S"),
        FUNCTION("F"),
        UNDEFINED("?");

        companion object {

            fun toType(value: Value) = when (value) {
                is Value.Boolean -> BOOLEAN
                is Value.Integer -> INTEGER
                is Value.Function -> FUNCTION
                Value.Nil -> NIL
                is Value.Decimal -> DECIMAL
                is Value.String -> STRING
            }
        }
    }

    data class TracedUse(val used: Boolean) {

        override fun toString() = if (used) {
            "#"
        } else {
            "."
        }
    }
}

