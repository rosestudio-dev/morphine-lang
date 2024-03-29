package ru.unit.morphine.assembly.optimizer.tracer

import org.jgrapht.graph.AbstractBaseGraph
import org.jgrapht.graph.DefaultEdge
import org.jgrapht.graph.DirectedAcyclicGraph
import org.jgrapht.traverse.DepthFirstIterator
import ru.unit.morphine.assembly.optimizer.exception.OptimizerException

class DominatorTree<T>(
    private val graph: AbstractBaseGraph<T, DefaultEdge>,
    start: T
) {
    val iDomGraph: DirectedAcyclicGraph<T, DefaultEdge>

    private val nodePreOrder = start?.let { first ->
        dfs(graph, first)
    } ?: emptyList()

    private val idomMap = mutableMapOf<T, T>()
    private val frontMap = mutableMapOf<T, MutableSet<T>>()
    private val preOrderMap = mutableMapOf<T, Int>()

    init {
        iDomGraph = create()
        calcFronts()
    }

    fun isDominates(dominator: T, dominated: T): Boolean {
        if (dominator == dominated) {
            return true
        }

        var dom = idomMap[dominated]

        while ((dom != null) && (preOrderMap[dom]!! >= preOrderMap[dominator]!!) && (dom != dominator)) {
            dom = idomMap[dom]!!
        }

        return dominator == dom
    }

    fun getIDom(t: T): T {
        return idomMap[t]!!
    }

    fun getFronts(t: T): Set<T> {
        return frontMap[t]!!.toSet()
    }

    private fun calcFronts() {
        idomMap.forEach { (dominated, _) ->
            frontMap[dominated] = mutableSetOf()
        }

        idomMap.forEach { (dominated, _) ->
            val subgraphVertexes = dfs(iDomGraph, dominated).toSet()

            val fronts = subgraphVertexes.flatMap { vertex ->
                graph.outgoingEdgesOf(vertex).mapNotNull { edge ->
                    val target = graph.getEdgeTarget(edge)

                    target
                }
            }.filter { front ->
                val s = graph.outgoingEdgesOf(front).any { edge -> graph.getEdgeTarget(edge) == dominated }
                front !in subgraphVertexes || s
            }.toSet()

            fronts.forEach { front ->
                frontMap[front]!!.add(dominated)
            }
        }
    }

    private fun create(): DirectedAcyclicGraph<T, DefaultEdge> {
        nodePreOrder.forEachIndexed { index, node ->
            preOrderMap[node] = index
        }

        computeDominators()

        return getDominatorTree()
    }

    private fun dfs(graph: AbstractBaseGraph<T, DefaultEdge>, startNode: T): List<T> {
        val iter = DepthFirstIterator(graph, startNode).apply {
            isCrossComponentTraversal = false
        }

        val travel = mutableListOf<T>()

        while (iter.hasNext()) {
            travel.add(iter.next())
        }

        return travel
    }

    private fun computeDominators() {
        val firstElement = nodePreOrder.firstOrNull() ?: return
        idomMap[firstElement] = firstElement

        var changed: Boolean

        do {
            changed = false

            for (node in nodePreOrder) {
                if (node == firstElement) {
                    continue
                }

                val oldIdom = idomMap[node]
                var newIdom: T? = null

                for (edge in graph.incomingEdgesOf(node)) {
                    val preNode = graph.getEdgeSource(edge)

                    if (idomMap[preNode] == null) {
                        continue
                    }

                    if (newIdom == null) {
                        newIdom = preNode
                    } else {
                        newIdom = intersectIDoms(preNode, newIdom)
                    }
                }

                if (newIdom == null) {
                    throw OptimizerException("null idom ($node)")
                }

                if (newIdom != oldIdom) {
                    changed = true

                    idomMap[node] = newIdom
                }
            }
        } while (changed)
    }

    private fun intersectIDoms(a: T, b: T): T? {
        var t1: T? = a
        var t2: T? = b

        while (t1 != t2) {
            if (preOrderMap[t1]!! < preOrderMap[t2]!!) {
                t2 = idomMap[t2]
            } else {
                t1 = idomMap[t1]
            }
        }

        return t1
    }

    private fun getDominatorTree(): DirectedAcyclicGraph<T, DefaultEdge> {
        val tree = DirectedAcyclicGraph<T, DefaultEdge>(DefaultEdge::class.java)
        for (node in graph.vertexSet()) {
            tree.addVertex(node)

            val idom = idomMap[node] ?: continue

            if (node != idom) {
                tree.addVertex(idom)
                tree.addEdge(idom, node)
            }
        }

        return tree
    }
}
