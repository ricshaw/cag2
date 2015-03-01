/*
 * TreeBuilder.cpp
 *
 *  Created on: 18 Feb 2015
 *      Author: gdp24
 */

#include "TreeHandler.h"

#include "ExtraFun.h"

#include <iostream>
using std::cout;
using std::endl;

TreeHandler::TreeHandler() {
	cTurn = 'p';
}

TreeHandler::~TreeHandler() {
}

void TreeHandler::buildTree(tree<State>& tr, char turn) {
	State emptyNode;
	NodeIt root, nodeIt;
	unsigned int prevCardNum = 0;

	// Insert a root node with an empty board for convenience
	root = tr.insert(tr.begin(), emptyNode);

	resetCardDeck();

	for (unsigned int i = 0; i < cards.size(); i++) {
		if (cards.at(i) != prevCardNum) {
			nodeIt = tr.append_child(root, emptyNode);
			buildNode(tr, nodeIt, turn, TAKE, i);
		}
		prevCardNum = cards.at(i);
	}
}

SiblingIt TreeHandler::getNextMove(char turn, float epsilon,
		const SiblingIt& startNode) {

	cTurn = turn;

	if (cTurn == 'p' || randf() > epsilon) {
		// Get next move with max V
		return getNextOptimalNode(startNode);
	} else {
		return getNextExploreNode(startNode);
	}
}

void TreeHandler::updateV(float alpha) {
	NodeIt parentNode, currentNode;

	while (!optimalMoveStack.empty()) {
		currentNode = optimalMoveStack.top();
		parentNode = optimalMoveParentStack.top();
		parentNode->setV(
				parentNode->getV()
						+ alpha * (currentNode->getV() - parentNode->getV()));
		optimalMoveStack.pop();
		optimalMoveParentStack.pop();
	}
}

void TreeHandler::buildNode(tree<State>& tr, NodeIt nodeIt, char turn, Action a,
		unsigned int cardNum) {

	if (a == SETTLE) {
		cout << "a: Settle, turn: " << nodeIt->getTurn() << ", cardSum: "
				<< nodeIt->getCardSum() << ", card: " << cards.at(cardNum);
	} else {
		cout << "a: Take, turn: " << nodeIt->getTurn() << ", cardSum: "
				<< nodeIt->getCardSum() << ", card: " << cards.at(cardNum);
	}

	unsigned int cardVal = cards.at(cardNum);
	cards.erase(cards.begin() + cardNum);
	nodeIt->addCard(cardVal);
	nodeIt->setA(a);

	if (turn == 'p') {
		nodeIt->setV(0.5);
	} else {
		nodeIt->setV(float(rand() % 9));
	}

	nodeIt->computeFinalState(turn);

	cout << " sumAfter: " << nodeIt->getCardSum() << " r: " << nodeIt->getR()
			<< endl;

	if (nodeIt->isFinalState()) {
		return;
	}

	switch (nodeIt->getSettleState()) {
	case BOTH_TAKING: {
		turn = switchTurn(turn);
		//playing as normal
		break;
	}
	case ONE_SETTLE: {
		//play only current
		break;
	}
	case BOTH_SETTLE: {
		//end game
		break;
	}
	}

	State newNode;
	if (nodeIt.node->parent && nodeIt.node->parent != tr.begin().node) {
		newNode = State(nodeIt.node->parent->data);
	}
	newNode.setTurn(turn);

	unsigned int prevCardNum = 0;
	NodeIt node1;
	for (unsigned int i = 0; i < cards.size(); i++) {
		if (prevCardNum != cards.at(i)) {
			node1 = tr.append_child(nodeIt, newNode);
			buildNode(tr, node1, turn, TAKE, i);
		}
		prevCardNum = cards.at(i);
	}

	//for (unsigned int i = 0; i < cards.size(); i++) {
	//	node1 = tr.append_child(nodeIt, newNode);
	//	buildNode(tr, node1, turn, SETTLE, i);
	//}

	cards.insert(cards.begin() + cardNum, cardVal);
}

SiblingIt TreeHandler::getNextOptimalNode(const SiblingIt& startNode) {

	SiblingIt nextSib = startNode, nextNode = startNode;

	// Advance to compare with the next sibling
	nextSib++;

	// Find the sibling with the bigger V
	for (; nextSib != startNode.end(); ++nextSib) {
		if (nextSib->getV() > nextNode->getV()) {
			nextNode = nextSib;
		}
	}

	if (cTurn == 'x' && nextNode.node->parent
			&& nextNode.node->parent->parent) {
		optimalMoveStack.push(nextNode);
		optimalMoveParentStack.push(nextNode.node->parent->parent);
	} else if (cTurn == 'o' && nextNode.number_of_children() == 0) {
		optimalMoveStack.push(nextNode);
		optimalMoveParentStack.push(nextNode.node->parent);
	}
	return nextNode;
}

SiblingIt TreeHandler::getNextExploreNode(const SiblingIt& startNode) {

	SiblingIt nextNode = startNode;

	unsigned int numSibling = NodeIt(startNode.parent_).number_of_children();

	// Pick a sibling at random
	numSibling = rand() % numSibling;

	nextNode += numSibling;

	return nextNode;
}

float TreeHandler::randf() {
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void TreeHandler::resetCardDeck() {
	cards.clear();
	cards.reserve(40);

	for (unsigned int i = 1; i <= 4; i++) {
		//Ace to 7
		for (unsigned int j = 1; j <= 7; j++) {
			cards.push_back(j);
		}
		//Jack to King
		for (unsigned int j = 1; j <= 3; j++) {
			cards.push_back(j + 9);
		}
	}
}
