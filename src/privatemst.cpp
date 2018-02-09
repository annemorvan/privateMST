#include <Rcpp.h>

//[[Rcpp::depends(BH)]]
//[[Rcpp::plugins(cpp11)]]


#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <limits.h>
#include <random>

#include <boost/graph/copy.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/utility.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
using namespace Rcpp;

// [[Rcpp::export]]
List rcpp_hello_world() {

    CharacterVector x = CharacterVector::create( "foo", "bar" )  ;
    NumericVector y   = NumericVector::create( 0.0, 1.0 ) ;
    List z            = List::create( x, y ) ;

    return z ;
}

// function to transform the Data frame that R will
// provide into a NumericMatrix for c++ to understand it
Rcpp::NumericMatrix DFtoNM( Rcpp::DataFrame x) {
  int nRows=x.nrows();
  Rcpp::NumericMatrix y(nRows,x.size());
  for (int i=0; i<x.size();i++) {
    y( Rcpp::_,i) = Rcpp::NumericVector(x[i]); // y( Rcpp::_,i) is a manner of choosing only the columns of y according to i
  }
  return y;
}

//' Leading NA
//'
//' This function returns a logical vector identifying if
//' there are leading NA, marking the leadings NA as TRUE and
//' everything else as FALSE.
//'
//' @param x An integer vector
//' @export
// [[Rcpp::export]]
LogicalVector leading_na(IntegerVector x) {
  int n = x.size();
  LogicalVector leading_na(n);

  int i = 0;
  while((i < n) &&(x[i] == NA_INTEGER)) {
    leading_na[i] = TRUE;
    i++;
  }
  return leading_na;
}



struct Nodeclass{ // define the characteristics of a node (here it has only a name)
  std::string name;
};

struct Edgeclass{ // define the characteristics of an edge (here it has only a weight, given that its name will be denoted by the two nodes forming the edge)
  float weight;
};

typedef boost::adjacency_list<boost::listS,boost::vecS,boost::undirectedS,Nodeclass,Edgeclass> Graph; // basic construction of a graph using boost::adjacency_list according to the characteristics we gave above
typedef Graph::vertex_descriptor NodeID; // vertex and edges descriptors are basics from  boost::graph that will help us call the node by their name and the edges by a couple of nodes forming it
typedef Graph::edge_descriptor EdgeID;


Rcpp::NumericMatrix approximated_MST(Graph const& g,double const& privacy_parameter ){ // The main function taking th graph g and a privacy_parameter
  // that is in fact the parameter at which the exponential distribution have to be parametrized such that the mechanism is an exponential mechanism

  unsigned long int N_Vertex= num_vertices(g) ;
  unsigned long int counter =0;

  Rcpp::NumericMatrix S_E (N_Vertex-1, 3);

  NodeID start = 0;
  std::unordered_set<NodeID> S_V ;
  S_V.insert(start);

  boost::graph_traits<Graph>::out_edge_iterator e,e_end;
  NodeID selected_node=start;
  EdgeID e_current;

  std::default_random_engine generator;
  std::exponential_distribution<double> rexp(privacy_parameter);

  std::vector<EdgeID> CurrentEdgeVector(N_Vertex);
  std::vector<double> Currentweight(N_Vertex);
  double candidate =-1;

  while(counter < (N_Vertex-1) ){// do it until every node has been labeled but one
    double current_weight=std::numeric_limits<float>::infinity(); // redefine current weight to be infinity
    std::fill(CurrentEdgeVector.begin(), CurrentEdgeVector.end(), e_current);
    std::fill(Currentweight.begin(), Currentweight.end(), current_weight);

    for ( auto current_node = S_V.begin(); current_node != S_V.end(); ++current_node){ // for every node in the set of labeled nodes
      for (boost::tie(e, e_end) = out_edges(*current_node, g); e != e_end; ++e){ // for every edge that is incident to this node

        if( S_V.find(target(*e,g)) == S_V.end()  ){ // verification that the edge is not between to nodes of S_V
          candidate= g[*e].weight - rexp(generator); // computing the noisy weight of the candidate
          if(candidate <= current_weight){// if the weight is better than the current better one, change places
            CurrentEdgeVector[*current_node]=*e;
            Currentweight[*current_node]=candidate;
            current_weight=candidate;
          }
        }

      }
    }

    for ( auto current_node = S_V.begin(); current_node != S_V.end(); ++current_node){
      if(Currentweight[*current_node]<=candidate){
        e_current=CurrentEdgeVector[*current_node];
      }
    }

    S_E(counter,0)= (double) source(e_current,g);
    S_E(counter,1)=(double) target(e_current,g); // on force la conversion en int pour que R comprenne ce qui se passe
    S_E(counter,2)=(double) g[e_current].weight;// add the edge with the min noisy value to the selected edges

    selected_node=source(e_current, g); // select the node forming the selected edge that is not already in S_V and insert it in the set of node labeled
    if( S_V.find(selected_node) == S_V.end()) {
      S_V.insert(selected_node);
    }else{
      selected_node=target(e_current, g);
      S_V.insert(selected_node);
    }

    ++counter;
  }
  //std::cout<<"remplissage du MST ok"<<std::endl;
  return(S_E);
}

// [[Rcpp::export]]
Rcpp::NumericMatrix PrivateMST(int order, Rcpp::DataFrame Elist, double privacydegree){

  Rcpp::NumericMatrix Edgelist = DFtoNM( Elist); // transform the edgelist (dataframe type) to a
  //NumericMatrix type edgelist  for C++ to understand it

  int numberofnode = order-1;
  int numberofedge =  Edgelist.nrow();

  Graph g(order) ;

  for (int i=0;i<numberofedge;++i) {// filling the weights and creating the edges

    NodeID u = Edgelist(i,0)-1; // setting the nodes that the edge descriptor will use
    NodeID v = Edgelist(i,1)-1;
    EdgeID edge; // initializing an edge descriptor

    bool ok;
    boost::tie(edge, ok) = boost::add_edge(u,v, g); // boost::add_edge return std::pair<EdgeID,bool>,tie do it for us

    if(Edgelist(i,2)!=0){ // if the weight is not 0 fill the weight
      if (ok){  // also if the edge creation worked
        g[edge].weight = Edgelist(i,2);
      }
    }
  }

  std::cout<< "Remplissage du graph OK" <<std::endl;
  std::cout<<"-------------------------"<<std::endl;


  Rcpp::NumericMatrix mstexpmechanism=approximated_MST(g,(privacydegree*numberofedge)/(2*(numberofnode)));
  //call the c++ function to approximate the MST using prim+exponential mechanism,
  //the privacy parameter here is the parameter that the exponential distribution
  //will use therefore it is epsilon'/2*delta_u (remeber that we use epsilon'=epsilon/(Nmber of nodes-1))
  return(mstexpmechanism);
}




