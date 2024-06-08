package morphine.optimizer.tracer

import morphine.bytecode.AbstractInstruction
import morphine.bytecode.Argument
import morphine.bytecode.Bytecode
import morphine.bytecode.Instruction
import morphine.optimizer.tracer.functions.controlFlow
import morphine.optimizer.tracer.functions.reconstructFunction
import morphine.optimizer.tracer.functions.toData
import morphine.optimizer.tracer.functions.traceDestinations
import morphine.optimizer.tracer.functions.traceValues
import morphine.optimizer.tracer.functions.traceVersions

class Tracer {

    fun trace(function: Bytecode.Function): Data {
        val data = function.toData()

        return if (function.instructions.isEmpty()) {
            data
        } else {
            data.controlFlow().apply {
                traceDestinations()
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
        lateinit var controlFlowTree: ControlFlowTree
    }

    data class Block(
        val id: Int,
        val start: Int,
        val end: Int,
    ) {

        val range = start until end

        fun nodes(data: Data) = data.nodes.subList(start, end)

        fun edges(data: Data): List<Block> {
            val positions = when (val endNode = data.nodes.getOrNull(end - 1)?.abstract?.instruction) {
                is Instruction.Jump -> listOf(endNode.position.value)
                is Instruction.JumpIf -> listOf(endNode.ifPosition.value, endNode.elsePosition.value)
                is Instruction.Leave -> listOf(data.nodes.size)
                else -> listOf(end)
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
        var destinations: Set<Int>
    ) {
        var isAvailable = true

        fun getSourceVersion(source: Argument.Slot) = tracedVersionsBefore[source.value]
        fun getDestVersion(source: Argument.Slot) = tracedVersionsAfter[source.value]

        fun getSourceValue(source: Argument.Slot) = tracedValuesBefore[source.value]
        fun getDestValue(source: Argument.Slot) = tracedValuesAfter[source.value]
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
            val index: Int,
            val constant: Int
        ) : TracedValue {

            override fun toString() = "c$constant:$index"
        }

        data class Phi(val values: Set<TracedValue>) : TracedValue {

            override fun toString() = "(${values.joinToString()})"
        }
    }
}
