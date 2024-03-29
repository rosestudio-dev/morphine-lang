package ru.unit.morphine.assembly.compiler.ast.node

sealed interface Expression : Node {

    fun <T : Compiler> eval(compiler: T) = compiler.eval(this)
}