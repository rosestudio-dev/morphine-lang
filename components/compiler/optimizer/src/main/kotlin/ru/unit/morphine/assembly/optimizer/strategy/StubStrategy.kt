package ru.unit.morphine.assembly.optimizer.strategy

import ru.unit.morphine.assembly.bytecode.Instruction
import ru.unit.morphine.assembly.optimizer.OptimizationStrategy
import ru.unit.morphine.assembly.optimizer.tracer.Tracer

class StubStrategy : OptimizationStrategy {

    override fun optimize(data: Tracer.Data): Tracer.Data {
        data.nodes.forEachIndexed { index, instructionWrapper ->
            val instruction = instructionWrapper.abstract.instruction

            if (instruction is Instruction.Jump && instruction.position.value == (index + 1)) {
                instructionWrapper.isAvailable = false
            }

            if (instruction is Instruction.Move && instruction.source == instruction.destination) {
                instructionWrapper.isAvailable = false
            }
        }

        return data
    }
}