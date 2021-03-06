% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/privateMSTlaplace.R
\name{laplaceMST}
\alias{laplaceMST}
\title{Compute a private MST using the Laplace mechanism}
\usage{
laplaceMST(graph, epsilon)
}
\arguments{
\item{graph}{igraph graph object from which a private Minimum Spanning Tree should be computed}

\item{epsilon}{privacy parameter of the algorithm}
}
\value{
Approximate MST using Laplace mechanism
}
\description{
Compute a private MST using the Laplace mechanism
}
\examples{
n <- 70
prob <- 0.1
## Generate random Erdos-Renyi graph
graph <- erdos.renyi.game(n, prob, type="gnp",directed = FALSE, loops = FALSE)
## Assign random weights to the edges, using an uniform probability distribution
E(graph)$weight <- runif(ecount(graph),0,10)
eps <- 0.6
approxMSTlaplace <- laplaceMST(graph, epsilon = eps)
print(sum(E(approxMSTlaplace)$weight))
print(sum(E(mst(graph))$weight))

## plot the resulting MST
mylayout <- layout.auto(graph)
par(mfrow=c(1,2))
plot(graph, layout=mylayout, vertex.size=5, vertex.label=NA)
plot(approxMSTlaplace, layout=mylayout, vertex.size=5, vertex.label=NA)

}
\references{
\insertRef{Sealfon_2016}{privateMST}
}
