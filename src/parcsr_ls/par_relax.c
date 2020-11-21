/******************************************************************************
 * Copyright 1998-2019 Lawrence Livermore National Security, LLC and other
 * HYPRE Project Developers. See the top-level COPYRIGHT file for details.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR MIT)
 ******************************************************************************/

/******************************************************************************
 *
 * Relaxation scheme
 *
 *****************************************************************************/

#include "_hypre_parcsr_ls.h"
#include "Common.h"
#include "_hypre_lapack.h"
#include "../sstruct_ls/gselim.h"


HYPRE_Int hypre_BoomerAMGRelax0WeightedJacobi( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, hypre_ParVector *u, hypre_ParVector *Vtemp );

HYPRE_Int hypre_BoomerAMGRelax1GaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, hypre_ParVector *u );

HYPRE_Int hypre_BoomerAMGRelax2GaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, hypre_ParVector *u );

HYPRE_Int hypre_BoomerAMGRelax5ChaoticHybridGaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, hypre_ParVector *u );

HYPRE_Int hypre_BoomerAMGRelax3HybridGaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax4HybridGaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax6HybridSSOR( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax7Jacobi( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real *l1_norms, hypre_ParVector *u, hypre_ParVector *Vtemp );

HYPRE_Int hypre_BoomerAMGRelax8HybridL1SSOR( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, HYPRE_Real *l1_norms, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax10TopoOrderedGaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax13HybridL1GaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, HYPRE_Real *l1_norms, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax14HybridL1GaussSeidel( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real omega, HYPRE_Real *l1_norms, hypre_ParVector *u, hypre_ParVector *Vtemp, hypre_ParVector *Ztemp );

HYPRE_Int hypre_BoomerAMGRelax18WeightedL1Jacobi( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Int *cf_marker, HYPRE_Int relax_points, HYPRE_Real relax_weight, HYPRE_Real *l1_norms, hypre_ParVector *u, hypre_ParVector *Vtemp );

HYPRE_Int hypre_BoomerAMGRelax19GaussElim( hypre_ParCSRMatrix *A, hypre_ParVector *f, hypre_ParVector *u );

HYPRE_Int hypre_BoomerAMGRelax98GaussElimPivot( hypre_ParCSRMatrix *A, hypre_ParVector *f, hypre_ParVector *u );

HYPRE_Int hypre_BoomerAMGRelaxKaczmarz( hypre_ParCSRMatrix *A, hypre_ParVector *f, HYPRE_Real omega, HYPRE_Real *l1_norms, hypre_ParVector *u );

/*--------------------------------------------------------------------------
 * hypre_BoomerAMGRelax
 *--------------------------------------------------------------------------*/
HYPRE_Int
hypre_BoomerAMGRelax( hypre_ParCSRMatrix *A,
                      hypre_ParVector    *f,
                      HYPRE_Int          *cf_marker,
                      HYPRE_Int           relax_type,
                      HYPRE_Int           relax_points,
                      HYPRE_Real          relax_weight,
                      HYPRE_Real          omega,
                      HYPRE_Real         *l1_norms,
                      hypre_ParVector    *u,
                      hypre_ParVector    *Vtemp,
                      hypre_ParVector    *Ztemp )
{
   HYPRE_Int relax_error = 0;

   /*---------------------------------------------------------------------------------------
    * Switch statement to direct control based on relax_type:
    *     relax_type =  0 -> Jacobi or CF-Jacobi
    *     relax_type =  1 -> Gauss-Seidel <--- very slow, sequential
    *     relax_type =  2 -> Gauss_Seidel: interior points in parallel,
    *                                      boundary sequential
    *     relax_type =  3 -> hybrid: SOR-J mix off-processor, SOR on-processor
    *                               with outer relaxation parameters (forward solve)
    *     relax_type =  4 -> hybrid: SOR-J mix off-processor, SOR on-processor
    *                               with outer relaxation parameters (backward solve)
    *     relax_type =  5 -> hybrid: GS-J mix off-processor, chaotic GS on-node
    *     relax_type =  6 -> hybrid: SSOR-J mix off-processor, SSOR on-processor
    *                               with outer relaxation parameters
    *     relax_type =  7 -> Jacobi (uses Matvec), only needed in CGNR [GPU-supported]
    *     relax_type =  8 -> hybrid L1 Symm. Gauss-Seidel
    *     relax_type =  9 -> Direct solve, Gaussian elimination
    *     relax_type = 10 -> On-processor direct forward solve for matrices with
    *                        triangular structure (indices need not be ordered
    *                        triangular)
    *     relax_type = 11 -> Two Stage approximation to GS. Uses the strict lower
    *                        part of the diagonal matrix
    *     relax_type = 12 -> Two Stage approximation to GS. Uses the full diagonal
    *                        matrix
    *     relax_type = 13 -> hybrid L1 Gauss-Seidel forward solve
    *     relax_type = 14 -> hybrid L1 Gauss-Seidel backward solve
    *     relax_type = 15 -> CG
    *     relax_type = 16 -> Scaled Chebyshev
    *     relax_type = 17 -> FCF-Jacobi
    *     relax_type = 18 -> L1-Jacobi [GPU-supported]
    *     relax_type = 19 -> Direct Solve, (old version)
    *     relax_type = 20 -> Kaczmarz
    *     relax_type = 29 -> Direct solve: use gaussian elimination & BLAS
    *                        (with pivoting) (old version)
    *     relax_type = 98 -> Direct solve, Gaussian elimination
    *     relax_type = 99 -> Direct solve, Gaussian elimination
    *     relax_type = 199-> Direct solve, Gaussian elimination
    *-------------------------------------------------------------------------------------*/

   switch (relax_type)
   {
      case 0: /* Weighted Jacobi */
         hypre_BoomerAMGRelax0WeightedJacobi(A, f, cf_marker, relax_points, relax_weight, u, Vtemp);
         break;

      case 1: /* Gauss-Seidel VERY SLOW */
         hypre_BoomerAMGRelax1GaussSeidel(A, f, cf_marker, relax_points, u);
         break;

      case 2: /* Gauss-Seidel: relax interior points in parallel, boundary sequentially */
         hypre_BoomerAMGRelax2GaussSeidel(A, f, cf_marker, relax_points, u);
         break;

      /* Hybrid: Jacobi off-processor, Gauss-Seidel on-processor (forward loop) */
      case 3:
         hypre_BoomerAMGRelax3HybridGaussSeidel(A, f, cf_marker, relax_points, relax_weight, omega, u, Vtemp, Ztemp);
         break;

      case 4: /* Hybrid: Jacobi off-processor, Gauss-Seidel/SOR on-processor (backward loop) */
         hypre_BoomerAMGRelax4HybridGaussSeidel(A, f, cf_marker, relax_points, relax_weight, omega, u, Vtemp, Ztemp);
         break;

      case 5: /* Hybrid: Jacobi off-processor, chaotic Gauss-Seidel on-processor */
         hypre_BoomerAMGRelax5ChaoticHybridGaussSeidel(A, f, cf_marker, relax_points, u);
         break;

      case 6: /* Hybrid: Jacobi off-processor, Symm. Gauss-Seidel/SSOR on-processor with outer relaxation parameter */
         hypre_BoomerAMGRelax6HybridSSOR(A, f, cf_marker, relax_points, relax_weight, omega, u, Vtemp, Ztemp);
         break;

      case 7: /* Jacobi (uses ParMatvec) */
         hypre_BoomerAMGRelax7Jacobi(A, f, relax_points, relax_weight, l1_norms, u, Vtemp);
         break;

      case 8: /* hybrid L1 Symm. Gauss-Seidel */
         hypre_BoomerAMGRelax8HybridL1SSOR(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp);
         break;

      /* Hybrid: Jacobi off-processor, ordered Gauss-Seidel on-processor */
      case 10:
         hypre_BoomerAMGRelax10TopoOrderedGaussSeidel(A, f, cf_marker, relax_points, relax_weight, omega, u, Vtemp, Ztemp);
         break;

      case 11: /* Two Stage Gauss Seidel. Forward sweep only */
         /* TO DO */
         break;

      case 12: /* Two Stage Gauss Seidel. Uses the diagonal matrix for the GS part */
         /* TO DO */
         break;

      case 13: /* hybrid L1 Gauss-Seidel forward solve */
         hypre_BoomerAMGRelax13HybridL1GaussSeidel(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp);
         break;

      case 14: /* hybrid L1 Gauss-Seidel backward solve */
         hypre_BoomerAMGRelax14HybridL1GaussSeidel(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp);
         break;

      case 18: /* weighted L1 Jacobi */
         hypre_BoomerAMGRelax18WeightedL1Jacobi(A, f, cf_marker, relax_points, relax_weight, l1_norms, u, Vtemp);
         break;

      case 19: /* Direct solve: use gaussian elimination */
         relax_error = hypre_BoomerAMGRelax19GaussElim(A, f, u);
         break;

      case 20: /* Kaczmarz */
         hypre_BoomerAMGRelaxKaczmarz(A, f, omega, l1_norms, u);
         break;

      case 98: /* Direct solve: use gaussian elimination & BLAS (with pivoting) */
         relax_error = hypre_BoomerAMGRelax98GaussElimPivot(A, f, u);
         break;
   }

   return relax_error;
}

HYPRE_Int
hypre_BoomerAMGRelaxWeightedJacobi_core( hypre_ParCSRMatrix *A,
                                         hypre_ParVector    *f,
                                         HYPRE_Int          *cf_marker,
                                         HYPRE_Int           relax_points,
                                         HYPRE_Real          relax_weight,
                                         HYPRE_Real         *l1_norms,
                                         hypre_ParVector    *u,
                                         hypre_ParVector    *Vtemp,
                                         HYPRE_Int           Skip_diag )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   hypre_Vector        *Vtemp_local   = hypre_ParVectorLocalVector(Vtemp);
   HYPRE_Complex       *Vtemp_data    = hypre_VectorData(Vtemp_local);
   HYPRE_Complex       *v_ext_data    = NULL;
   HYPRE_Complex       *v_buf_data    = NULL;

   HYPRE_Complex        zero             = 0.0;
   HYPRE_Real           one_minus_weight = 1.0 - relax_weight;
   HYPRE_Complex        res;

   HYPRE_Int num_procs, my_id, i, j, ii, jj, index, num_sends, start;
   hypre_ParCSRCommHandle *comm_handle;

   /* use l1_norm or diagonal */
   const HYPRE_Int use_l1_norms = l1_norms ? 1 : 0;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   if (num_procs > 1)
   {
      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      v_buf_data = hypre_CTAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends), HYPRE_MEMORY_HOST);
      v_ext_data = hypre_CTAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);

      index = 0;
      for (i = 0; i < num_sends; i++)
      {
         start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
         for (j = start; j < hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1); j++)
         {
            v_buf_data[index++] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg, j)];
         }
      }

      comm_handle = hypre_ParCSRCommHandleCreate(1, comm_pkg, v_buf_data, v_ext_data);
   }

   /*-----------------------------------------------------------------
    * Copy current approximation into temporary vector.
    *-----------------------------------------------------------------*/
#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i) HYPRE_SMP_SCHEDULE
#endif
   for (i = 0; i < num_rows; i++)
   {
      Vtemp_data[i] = u_data[i];
   }

   if (num_procs > 1)
   {
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = NULL;
   }

   /*-----------------------------------------------------------------
    * Relax all points.
    *-----------------------------------------------------------------*/
#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i,ii,jj,res) HYPRE_SMP_SCHEDULE
#endif
   for (i = 0; i < num_rows; i++)
   {
      const HYPRE_Complex di = use_l1_norms ? l1_norms[i] : A_diag_data[A_diag_i[i]];

      /*-----------------------------------------------------------
       * If i is of the right type ( C or F or All ) and diagonal is
       * nonzero, relax point i; otherwise, skip it.
       * Relax only C or F points as determined by relax_points.
       *-----------------------------------------------------------*/
      if ( (relax_points == 0 || cf_marker[i] == relax_points) && di != zero )
      {
         res = f_data[i];
         for (jj = A_diag_i[i] + Skip_diag; jj < A_diag_i[i+1]; jj++)
         {
            ii = A_diag_j[jj];
            res -= A_diag_data[jj] * Vtemp_data[ii];
         }
         for (jj = A_offd_i[i]; jj < A_offd_i[i+1]; jj++)
         {
            ii = A_offd_j[jj];
            res -= A_offd_data[jj] * v_ext_data[ii];
         }

         if (Skip_diag)
         {
            u_data[i] *= one_minus_weight;
            u_data[i] += relax_weight * res / di;
         }
         else
         {
            u_data[i] += relax_weight * res / di;
         }
      }
   }

   if (num_procs > 1)
   {
      hypre_TFree(v_ext_data, HYPRE_MEMORY_HOST);
      hypre_TFree(v_buf_data, HYPRE_MEMORY_HOST);
   }

   return hypre_error_flag;
}

HYPRE_Int
hypre_BoomerAMGRelax0WeightedJacobi( hypre_ParCSRMatrix *A,
                                     hypre_ParVector    *f,
                                     HYPRE_Int          *cf_marker,
                                     HYPRE_Int           relax_points,
                                     HYPRE_Real          relax_weight,
                                     hypre_ParVector    *u,
                                     hypre_ParVector    *Vtemp )
{
   return hypre_BoomerAMGRelaxWeightedJacobi_core(A, f, cf_marker, relax_points, relax_weight, NULL, u, Vtemp, 1);
}

HYPRE_Int
hypre_BoomerAMGRelax18WeightedL1Jacobi( hypre_ParCSRMatrix *A,
                                        hypre_ParVector    *f,
                                        HYPRE_Int          *cf_marker,
                                        HYPRE_Int           relax_points,
                                        HYPRE_Real          relax_weight,
                                        HYPRE_Real         *l1_norms,
                                        hypre_ParVector    *u,
                                        hypre_ParVector    *Vtemp )
{
#if defined(HYPRE_USING_CUDA)
   //HYPRE_ExecutionPolicy exec = hypre_GetExecPolicy2( hypre_VectorMemoryLocation(x), hypre_VectorMemoryLocation(b) );
   //RL: TODO back to hypre_GetExecPolicy2 later
   HYPRE_ExecutionPolicy exec = HYPRE_EXEC_DEVICE;
   if (exec == HYPRE_EXEC_DEVICE)
   {
      // XXX GPU calls Relax7 XXX
      return hypre_BoomerAMGRelax7Jacobi(A, f, relax_points, relax_weight, l1_norms, u, Vtemp);
   }
   else
#endif
   {
      return hypre_BoomerAMGRelaxWeightedJacobi_core(A, f, cf_marker, relax_points, relax_weight, l1_norms, u, Vtemp, 0);
   }
}

HYPRE_Int
hypre_BoomerAMGRelax1GaussSeidel( hypre_ParCSRMatrix *A,
                                  hypre_ParVector    *f,
                                  HYPRE_Int          *cf_marker,
                                  HYPRE_Int           relax_points,
                                  hypre_ParVector    *u )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   HYPRE_Complex       *v_ext_data    = NULL;
   HYPRE_Complex       *v_buf_data    = NULL;
   HYPRE_Complex        zero          = 0.0;
   HYPRE_Complex        res;

   HYPRE_Int num_procs, my_id, i, j, ii, jj, p, jr, ip, num_sends, num_recvs, vec_start, vec_len;
   hypre_MPI_Status *status;
   hypre_MPI_Request *requests;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   if (num_procs > 1)
   {
      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);

      v_buf_data = hypre_CTAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends), HYPRE_MEMORY_HOST);
      v_ext_data = hypre_CTAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);

      status = hypre_CTAlloc(hypre_MPI_Status, num_recvs+num_sends, HYPRE_MEMORY_HOST);
      requests = hypre_CTAlloc(hypre_MPI_Request, num_recvs+num_sends, HYPRE_MEMORY_HOST);
   }

   /*-----------------------------------------------------------------
    * Relax all points.
    *-----------------------------------------------------------------*/
   for (p = 0; p < num_procs; p++)
   {
      jr = 0;
      if (p != my_id)
      {
         for (i = 0; i < num_sends; i++)
         {
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            if (ip == p)
            {
               vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
               vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1) - vec_start;
               for (j = vec_start; j < vec_start+vec_len; j++)
               {
                  v_buf_data[j] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg,j)];
               }
               hypre_MPI_Isend(&v_buf_data[vec_start], vec_len, HYPRE_MPI_REAL, ip, 0, comm, &requests[jr++]);
            }
         }
         hypre_MPI_Waitall(jr, requests, status);
         hypre_MPI_Barrier(comm);
      }
      else
      {
         if (num_procs > 1)
         {
            for (i = 0; i < num_recvs; i++)
            {
               ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
               vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
               vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1) - vec_start;
               hypre_MPI_Irecv(&v_ext_data[vec_start], vec_len, HYPRE_MPI_REAL, ip, 0, comm, &requests[jr++]);
            }
            hypre_MPI_Waitall(jr, requests, status);
         }

         for (i = 0; i < num_rows; i++)
         {
            /*-----------------------------------------------------------
             * If i is of the right type ( C or F ) and diagonal is
             * nonzero, relax point i; otherwise, skip it.
             * Relax only C or F points as determined by relax_points.
             *-----------------------------------------------------------*/
            if ( (relax_points == 0 || cf_marker[i] == relax_points) && A_diag_data[A_diag_i[i]] != zero )
            {
               res = f_data[i];
               for (jj = A_diag_i[i] + 1; jj < A_diag_i[i+1]; jj++)
               {
                  ii = A_diag_j[jj];
                  res -= A_diag_data[jj] * u_data[ii];
               }
               for (jj = A_offd_i[i]; jj < A_offd_i[i+1]; jj++)
               {
                  ii = A_offd_j[jj];
                  res -= A_offd_data[jj] * v_ext_data[ii];
               }
               u_data[i] = res / A_diag_data[A_diag_i[i]];
            }
         }

         if (num_procs > 1)
         {
            hypre_MPI_Barrier(comm);
         }
      }
   }

   if (num_procs > 1)
   {
      hypre_TFree(v_ext_data, HYPRE_MEMORY_HOST);
      hypre_TFree(v_buf_data, HYPRE_MEMORY_HOST);
      hypre_TFree(status, HYPRE_MEMORY_HOST);
      hypre_TFree(requests, HYPRE_MEMORY_HOST);
   }

   return hypre_error_flag;
}

HYPRE_Int
hypre_BoomerAMGRelax2GaussSeidel( hypre_ParCSRMatrix *A,
                                  hypre_ParVector    *f,
                                  HYPRE_Int          *cf_marker,
                                  HYPRE_Int           relax_points,
                                  hypre_ParVector    *u )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   HYPRE_Complex       *v_ext_data    = NULL;
   HYPRE_Complex       *v_buf_data    = NULL;
   HYPRE_Complex        zero          = 0.0;
   HYPRE_Complex        res;

   HYPRE_Int num_procs, my_id, i, j, ii, jj, p, jr, ip, num_sends, num_recvs, vec_start, vec_len;
   hypre_MPI_Status *status;
   hypre_MPI_Request *requests;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   if (num_procs > 1)
   {
      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);

      v_buf_data = hypre_CTAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends), HYPRE_MEMORY_HOST);
      v_ext_data = hypre_CTAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);

      status  = hypre_CTAlloc(hypre_MPI_Status, num_recvs+num_sends, HYPRE_MEMORY_HOST);
      requests = hypre_CTAlloc(hypre_MPI_Request, num_recvs+num_sends, HYPRE_MEMORY_HOST);
   }

   /*-----------------------------------------------------------------
    * Relax interior points first
    *-----------------------------------------------------------------*/
   for (i = 0; i < num_rows; i++)
   {
      /*-----------------------------------------------------------
       * If i is of the right type ( C or F or All ) and diagonal is
       * nonzero, relax point i; otherwise, skip it.
       *-----------------------------------------------------------*/
      if ( (relax_points == 0 || cf_marker[i] == relax_points) && A_offd_i[i+1] - A_offd_i[i] == zero &&
            A_diag_data[A_diag_i[i]] != zero )
      {
         res = f_data[i];
         for (jj = A_diag_i[i] + 1; jj < A_diag_i[i+1]; jj++)
         {
            ii = A_diag_j[jj];
            res -= A_diag_data[jj] * u_data[ii];
         }
         u_data[i] = res / A_diag_data[A_diag_i[i]];
      }
   }

   for (p = 0; p < num_procs; p++)
   {
      jr = 0;
      if (p != my_id)
      {
         for (i = 0; i < num_sends; i++)
         {
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            if (ip == p)
            {
               vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
               vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1) - vec_start;
               for (j = vec_start; j < vec_start+vec_len; j++)
               {
                  v_buf_data[j] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg,j)];
               }
               hypre_MPI_Isend(&v_buf_data[vec_start], vec_len, HYPRE_MPI_REAL, ip, 0, comm, &requests[jr++]);
            }
         }
         hypre_MPI_Waitall(jr, requests, status);
         hypre_MPI_Barrier(comm);
      }
      else
      {
         if (num_procs > 1)
         {
            for (i = 0; i < num_recvs; i++)
            {
               ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
               vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
               vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1) - vec_start;
               hypre_MPI_Irecv(&v_ext_data[vec_start], vec_len, HYPRE_MPI_REAL, ip, 0, comm, &requests[jr++]);
            }
            hypre_MPI_Waitall(jr, requests, status);
         }
         for (i = 0; i < num_rows; i++)
         {
            /*-----------------------------------------------------------
             * If i is of the right type ( C or F or All) and diagonal is
             * nonzero, relax point i; otherwise, skip it.
             * Relax only C or F points as determined by relax_points.
             *-----------------------------------------------------------*/
            if ( (relax_points == 0 || cf_marker[i] == relax_points) && A_offd_i[i+1] - A_offd_i[i] != zero &&
                  A_diag_data[A_diag_i[i]] != zero)
            {
               res = f_data[i];
               for (jj = A_diag_i[i]+1; jj < A_diag_i[i+1]; jj++)
               {
                  ii = A_diag_j[jj];
                  res -= A_diag_data[jj] * u_data[ii];
               }
               for (jj = A_offd_i[i]; jj < A_offd_i[i+1]; jj++)
               {
                  ii = A_offd_j[jj];
                  res -= A_offd_data[jj] * v_ext_data[ii];
               }
               u_data[i] = res / A_diag_data[A_diag_i[i]];
            }
         }
         if (num_procs > 1)
         {
            hypre_MPI_Barrier(comm);
         }
      }
   }
   if (num_procs > 1)
   {
      hypre_TFree(v_ext_data, HYPRE_MEMORY_HOST);
      hypre_TFree(v_buf_data, HYPRE_MEMORY_HOST);
      hypre_TFree(status, HYPRE_MEMORY_HOST);
      hypre_TFree(requests, HYPRE_MEMORY_HOST);
   }

   return hypre_error_flag;
}

HYPRE_Int
hypre_BoomerAMGRelaxHybridGaussSeidel_core( hypre_ParCSRMatrix *A,
                                            hypre_ParVector    *f,
                                            HYPRE_Int          *cf_marker,
                                            HYPRE_Int           relax_points,
                                            HYPRE_Real          relax_weight,
                                            HYPRE_Real          omega,
                                            HYPRE_Real         *l1_norms,
                                            hypre_ParVector    *u,
                                            hypre_ParVector    *Vtemp,
                                            hypre_ParVector    *Ztemp,
                                            HYPRE_Int           GS_order,
                                            HYPRE_Int           Symm,
                                            HYPRE_Int           Skip_diag,
                                            HYPRE_Int           Topo_order )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   hypre_Vector        *Vtemp_local   = hypre_ParVectorLocalVector(Vtemp);
   HYPRE_Complex       *Vtemp_data    = hypre_VectorData(Vtemp_local);
   /*
   hypre_Vector        *Ztemp_local   = NULL;
   HYPRE_Complex       *Ztemp_data    = NULL;
   */
   HYPRE_Complex       *v_ext_data    = NULL;
   HYPRE_Complex       *v_buf_data    = NULL;
   HYPRE_Int           *proc_ordering = NULL;

   HYPRE_Complex        zero             = 0.0;
   HYPRE_Real           one_minus_omega  = 1.0 - omega;
   HYPRE_Complex        res, res0, res2;

   HYPRE_Int num_procs, my_id, num_threads, i, j, ii, jj, num_sends;
   HYPRE_Int ns, ne, size, rest;
   HYPRE_Int sweep;
   hypre_ParCSRCommHandle *comm_handle;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);
   num_threads = hypre_NumThreads();

   /* GS order: forward or backward */
   const HYPRE_Int gs_order = GS_order > 0 ? 1 : -1;
   /* for symmetric GS, a forward followed by a backward */
   const HYPRE_Int num_sweeps = Symm ? 2 : 1;
   /* use l1_norm or diagonal */
   const HYPRE_Int use_l1_norms = l1_norms ? 1 : 0;
   /* if relax_weight and omega are both 1.0 */
   const HYPRE_Int non_scale = relax_weight == 1.0 && omega == 1.0;
   /* */
   const HYPRE_Real prod = 1.0 - relax_weight * omega;

   /*
   if (num_threads > 1)
   {
      Ztemp_local = hypre_ParVectorLocalVector(Ztemp);
      Ztemp_data  = hypre_VectorData(Ztemp_local);
   }
   */

#if defined(HYPRE_USING_PERSISTENT_COMM)
   // JSP: persistent comm can be similarly used for other smoothers
   hypre_ParCSRPersistentCommHandle *persistent_comm_handle;
#endif

   if (num_procs > 1)
   {
#ifdef HYPRE_PROFILE
      hypre_profile_times[HYPRE_TIMER_ID_PACK_UNPACK] -= hypre_MPI_Wtime();
#endif

      if (!comm_pkg)
      {
         hypre_MatvecCommPkgCreate(A);
         comm_pkg = hypre_ParCSRMatrixCommPkg(A);
      }

      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);

#if defined(HYPRE_USING_PERSISTENT_COMM)
      persistent_comm_handle = hypre_ParCSRCommPkgGetPersistentCommHandle(1, comm_pkg);
      v_buf_data = (HYPRE_Real *) hypre_ParCSRCommHandleSendDataBuffer(persistent_comm_handle);
      v_ext_data  = (HYPRE_Real *) hypre_ParCSRCommHandleRecvDataBuffer(persistent_comm_handle);
#else
      v_buf_data = hypre_CTAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends), HYPRE_MEMORY_HOST);
      v_ext_data = hypre_CTAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);
#endif

      HYPRE_Int begin = hypre_ParCSRCommPkgSendMapStart(comm_pkg, 0);
      HYPRE_Int end   = hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends);
#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for HYPRE_SMP_SCHEDULE
#endif
      for (i = begin; i < end; i++)
      {
         v_buf_data[i - begin] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg, i)];
      }

#ifdef HYPRE_PROFILE
      hypre_profile_times[HYPRE_TIMER_ID_PACK_UNPACK] += hypre_MPI_Wtime();
      hypre_profile_times[HYPRE_TIMER_ID_HALO_EXCHANGE] -= hypre_MPI_Wtime();
#endif

#if defined(HYPRE_USING_PERSISTENT_COMM)
      hypre_ParCSRPersistentCommHandleStart(persistent_comm_handle, HYPRE_MEMORY_HOST, v_buf_data);
#else
      comm_handle = hypre_ParCSRCommHandleCreate(1, comm_pkg, v_buf_data, v_ext_data);
#endif

#if defined(HYPRE_USING_PERSISTENT_COMM)
      hypre_ParCSRPersistentCommHandleWait(persistent_comm_handle, HYPRE_MEMORY_HOST, v_ext_data);
#else
      hypre_ParCSRCommHandleDestroy(comm_handle);
#endif
      comm_handle = NULL;

#ifdef HYPRE_PROFILE
      hypre_profile_times[HYPRE_TIMER_ID_HALO_EXCHANGE] += hypre_MPI_Wtime();
#endif
   }

   if (Topo_order)
   {
      /* Check for ordering of matrix. If stored, get pointer, otherwise
       * compute ordering and point matrix variable to array.
       * Used in AIR
       */
      if (!hypre_ParCSRMatrixProcOrdering(A))
      {
         proc_ordering = hypre_CTAlloc(HYPRE_Int, num_rows, HYPRE_MEMORY_HOST);
         hypre_topo_sort(A_diag_i, A_diag_j, A_diag_data, proc_ordering, num_rows);
         hypre_ParCSRMatrixProcOrdering(A) = proc_ordering;
      }
      else
      {
         proc_ordering = hypre_ParCSRMatrixProcOrdering(A);
      }
   }

   /*-----------------------------------------------------------------
    * Relax all points.
    *-----------------------------------------------------------------*/
#ifdef HYPRE_PROFILE
   hypre_profile_times[HYPRE_TIMER_ID_RELAX] -= hypre_MPI_Wtime();
#endif

#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i) HYPRE_SMP_SCHEDULE
#endif
   for (i = 0; i < num_rows; i++)
   {
      Vtemp_data[i] = u_data[i];
   }

   if (num_threads > 1)
   {
#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i,ii,j,jj,ns,ne,res,rest,size) HYPRE_SMP_SCHEDULE
#endif
      for (j = 0; j < num_threads; j++)
      {
         size = num_rows / num_threads;
         rest = num_rows - size * num_threads;
         if (j < rest)
         {
            ns = j * size + j;
            ne = (j + 1) * size + j + 1;
         }
         else
         {
            ns = j * size + rest;
            ne = (j + 1) * size + rest;
         }

         for (sweep = 0; sweep < num_sweeps; sweep++)
         {
            HYPRE_Int order;
            if (num_sweeps == 1)
            {
               order = gs_order;
            }
            else
            {
               order = sweep == 0 ? 1: -1;
            }

            hypre_Iterator *iter = hypre_IteratorCreate(ns, ne, order);
            for ( i = hypre_IteratorBegin(iter); i != hypre_IteratorEnd(iter); i += hypre_IteratorStep(iter) )
            {
               const HYPRE_Int row = proc_ordering ? proc_ordering[i] : i;
               const HYPRE_Complex di = use_l1_norms ? l1_norms[row] : A_diag_data[A_diag_i[row]];

               /*-----------------------------------------------------------
                * Relax only C or F points as determined by relax_points.
                * If row is of the right type ( C or F or All) and diagonal is
                * nonzero, relax point row; otherwise, skip it.
                *-----------------------------------------------------------*/
               if ( (relax_points == 0 || cf_marker[row] == relax_points) && di != zero )
               {
                  res = f_data[row];
                  res0 = 0.0;
                  res2 = 0.0;

                  for (jj = A_diag_i[row] + Skip_diag; jj < A_diag_i[row+1]; jj++)
                  {
                     ii = A_diag_j[jj];
                     if (ii >= ns && ii < ne)
                     {
                        if (non_scale)
                        {
                           res -= A_diag_data[jj] * u_data[ii];
                        }
                        else
                        {
                           res0 -= A_diag_data[jj] * u_data[ii];
                           res2 += A_diag_data[jj] * Vtemp_data[ii];
                        }
                     }
                     else
                     {
                        res -= A_diag_data[jj] * Vtemp_data[ii];
                     }
                  }

                  for (jj = A_offd_i[row]; jj < A_offd_i[row+1]; jj++)
                  {
                     ii = A_offd_j[jj];
                     res -= A_offd_data[jj] * v_ext_data[ii];
                  }

                  if (non_scale)
                  {
                     if (Skip_diag)
                     {
                        u_data[row] = res / di;
                     }
                     else
                     {
                        u_data[row] += res / di;
                     }
                  }
                  else
                  {
                     if (Skip_diag)
                     {
                        u_data[row] = u_data[row] * prod + relax_weight * (omega * res + res0 + one_minus_omega * res2) / di;
                     }
                     else
                     {
                        u_data[row] += relax_weight * (omega * res + res0 + one_minus_omega * res2) / di;
                     }
                  }
               }
            }
            hypre_IteratorDestroy(iter);
         } /* for (sweep = 0; sweep < num_sweeps; sweep++) */
      } /* for (j = 0; j < num_threads; j++) */
   }
   else /* if (num_threads > 1) */
   {
      for (sweep = 0; sweep < num_sweeps; sweep++)
      {
         HYPRE_Int order;
         if (num_sweeps == 1)
         {
            order = gs_order;
         }
         else
         {
            order = sweep == 0 ? 1: -1;
         }

         hypre_Iterator *iter = hypre_IteratorCreate(0, num_rows, order);
         for ( i = hypre_IteratorBegin(iter); i != hypre_IteratorEnd(iter); i += hypre_IteratorStep(iter) )
         {
            const HYPRE_Int row = proc_ordering ? proc_ordering[i] : i;
            const HYPRE_Complex di = use_l1_norms ? l1_norms[row] : A_diag_data[A_diag_i[row]];

            /*-----------------------------------------------------------
             * Relax only C or F points as determined by relax_points.
             * If row is of the right type ( C or F or All) and diagonal is
             * nonzero, relax point row; otherwise, skip it.
             *-----------------------------------------------------------*/
            if ( (relax_points == 0 || cf_marker[row] == relax_points) && di != zero )
            {
               res = f_data[row];
               res0 = 0.0;
               res2 = 0.0;

               for (jj = A_diag_i[row] + Skip_diag; jj < A_diag_i[row+1]; jj++)
               {
                  ii = A_diag_j[jj];
                  if (non_scale)
                  {
                     res -= A_diag_data[jj] * u_data[ii];
                  }
                  else
                  {
                     res0 -= A_diag_data[jj] * u_data[ii];
                     res2 += A_diag_data[jj] * Vtemp_data[ii];
                  }
               }

               for (jj = A_offd_i[row]; jj < A_offd_i[row+1]; jj++)
               {
                  ii = A_offd_j[jj];
                  res -= A_offd_data[jj] * v_ext_data[ii];
               }

               if (non_scale)
               {
                  if (Skip_diag)
                  {
                     u_data[row] = res / di;
                  }
                  else
                  {
                     u_data[row] += res / di;
                  }
               }
               else
               {
                  if (Skip_diag)
                  {
                     u_data[row] = u_data[row] * prod + relax_weight * (omega * res + res0 + one_minus_omega * res2) / di;
                  }
                  else
                  {
                     u_data[row] += relax_weight * (omega * res + res0 + one_minus_omega * res2) / di;
                  }
               }
            }
         }
         hypre_IteratorDestroy(iter);
      } /* for (sweep = 0; sweep < num_sweeps; sweep++) */
   } /* if (num_threads > 1) */

#ifndef HYPRE_USING_PERSISTENT_COMM
   if (num_procs > 1)
   {
      hypre_TFree(v_ext_data, HYPRE_MEMORY_HOST);
      hypre_TFree(v_buf_data, HYPRE_MEMORY_HOST);
   }
#endif

#ifdef HYPRE_PROFILE
   hypre_profile_times[HYPRE_TIMER_ID_RELAX] += hypre_MPI_Wtime();
#endif

   return hypre_error_flag;
}

/* forward hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax3HybridGaussSeidel( hypre_ParCSRMatrix *A,
                                        hypre_ParVector    *f,
                                        HYPRE_Int          *cf_marker,
                                        HYPRE_Int           relax_points,
                                        HYPRE_Real          relax_weight,
                                        HYPRE_Real          omega,
                                        hypre_ParVector    *u,
                                        hypre_ParVector    *Vtemp,
                                        hypre_ParVector    *Ztemp )
{
   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, NULL, u, Vtemp, Ztemp,
                                                     1 /* forward */,  0 /* nonsymm */, 1 /* skip diag */, 0);
}

/* backward hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax4HybridGaussSeidel( hypre_ParCSRMatrix *A,
                                        hypre_ParVector    *f,
                                        HYPRE_Int          *cf_marker,
                                        HYPRE_Int           relax_points,
                                        HYPRE_Real          relax_weight,
                                        HYPRE_Real          omega,
                                        hypre_ParVector    *u,
                                        hypre_ParVector    *Vtemp,
                                        hypre_ParVector    *Ztemp )
{
   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, NULL, u, Vtemp, Ztemp,
                                                     -1 /* backward */, 0 /* nosymm */, 1 /* skip diag */, 0);
}

/* chaotic forward G-S */
HYPRE_Int
hypre_BoomerAMGRelax5ChaoticHybridGaussSeidel( hypre_ParCSRMatrix *A,
                                               hypre_ParVector    *f,
                                               HYPRE_Int          *cf_marker,
                                               HYPRE_Int           relax_points,
                                               hypre_ParVector    *u )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   HYPRE_Complex       *v_ext_data    = NULL;
   HYPRE_Complex       *v_buf_data    = NULL;

   HYPRE_Complex        zero             = 0.0;
   HYPRE_Complex        res;

   HYPRE_Int num_procs, my_id, i, j, ii, jj, index, num_sends, start;
   hypre_ParCSRCommHandle *comm_handle;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   if (num_procs > 1)
   {
      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      v_buf_data = hypre_CTAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg,  num_sends), HYPRE_MEMORY_HOST);
      v_ext_data = hypre_CTAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);

      index = 0;
      for (i = 0; i < num_sends; i++)
      {
         start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
         for (j = start; j < hypre_ParCSRCommPkgSendMapStart(comm_pkg,i+1); j++)
         {
            v_buf_data[index++] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg,j)];
         }
      }

      comm_handle = hypre_ParCSRCommHandleCreate(1, comm_pkg, v_buf_data, v_ext_data);

      /*-----------------------------------------------------------------
       * Copy current approximation into temporary vector.
       *-----------------------------------------------------------------*/
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = NULL;
   }

#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i,ii,jj,res) HYPRE_SMP_SCHEDULE
#endif
   for (i = 0; i < num_rows; i++)
   {
      /*-----------------------------------------------------------
       * If i is of the right type ( C or F or All) and diagonal is
       * nonzero, relax point i; otherwise, skip it.
       * Relax only C or F points as determined by relax_points.
       *-----------------------------------------------------------*/
      if ( (relax_points == 0 || cf_marker[i] == relax_points) && A_diag_data[A_diag_i[i]] != zero )
      {
         res = f_data[i];
         for (jj = A_diag_i[i] + 1; jj < A_diag_i[i+1]; jj++)
         {
            ii = A_diag_j[jj];
            res -= A_diag_data[jj] * u_data[ii];
         }
         for (jj = A_offd_i[i]; jj < A_offd_i[i+1]; jj++)
         {
            ii = A_offd_j[jj];
            res -= A_offd_data[jj] * v_ext_data[ii];
         }
         u_data[i] = res / A_diag_data[A_diag_i[i]];
      }
   }

   if (num_procs > 1)
   {
      hypre_TFree(v_ext_data, HYPRE_MEMORY_HOST);
      hypre_TFree(v_buf_data, HYPRE_MEMORY_HOST);
   }

   return hypre_error_flag;
}

/* symmetric hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax6HybridSSOR( hypre_ParCSRMatrix *A,
                                 hypre_ParVector    *f,
                                 HYPRE_Int          *cf_marker,
                                 HYPRE_Int           relax_points,
                                 HYPRE_Real          relax_weight,
                                 HYPRE_Real          omega,
                                 hypre_ParVector    *u,
                                 hypre_ParVector    *Vtemp,
                                 hypre_ParVector    *Ztemp )
{
   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, NULL, u, Vtemp, Ztemp,
                                                     1, 1 /* symm */, 1 /* skip diag */, 0);
}

HYPRE_Int
hypre_BoomerAMGRelax7Jacobi( hypre_ParCSRMatrix *A,
                             hypre_ParVector    *f,
                             HYPRE_Int           relax_points,
                             HYPRE_Real          relax_weight,
                             HYPRE_Real         *l1_norms,
                             hypre_ParVector    *u,
                             hypre_ParVector    *Vtemp )
{
   HYPRE_Int       num_rows = hypre_ParCSRMatrixNumRows(A);
   hypre_Vector    l1_norms_vec;
   hypre_ParVector l1_norms_parvec;

   hypre_VectorData(&l1_norms_vec) = l1_norms;
   hypre_VectorSize(&l1_norms_vec) = num_rows;
   /* TODO XXX
    * The next line is NOT 100% correct, which should be the memory location of l1_norms instead of f
    * But how do I know it? As said, don't use raw pointers, don't use raw pointers!
    * It is fine normally since A, f, and l1_norms should live in the same memory space
    */
   hypre_VectorMemoryLocation(&l1_norms_vec) = hypre_ParVectorMemoryLocation(f);
   hypre_ParVectorLocalVector(&l1_norms_parvec) = &l1_norms_vec;

#if defined(HYPRE_USING_CUDA)
   HYPRE_Int sync_stream = hypre_HandleCudaComputeStreamSync(hypre_handle());
   hypre_HandleCudaComputeStreamSync(hypre_handle()) = 0;
#endif

   /*-----------------------------------------------------------------
    * Copy f into temporary vector.
    *-----------------------------------------------------------------*/
   hypre_ParVectorCopy(f, Vtemp);

   /*-----------------------------------------------------------------
    * Perform Matvec Vtemp = w * (f - Au)
    *-----------------------------------------------------------------*/
   hypre_ParCSRMatrixMatvec(-relax_weight, A, u, relax_weight, Vtemp);

   /*-----------------------------------------------------------------
    * u += D^{-1} * Vtemp, where D_ii = ||A(i,:)||_1
    *-----------------------------------------------------------------*/
   hypre_ParVectorElmdivpy(Vtemp, &l1_norms_parvec, u);

#if defined(HYPRE_USING_CUDA)
   hypre_HandleCudaComputeStreamSync(hypre_handle()) = sync_stream;
   hypre_SyncCudaComputeStream(hypre_handle());
#endif

   return hypre_error_flag;
}

/* symmetric l1 hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax8HybridL1SSOR( hypre_ParCSRMatrix *A,
                                   hypre_ParVector    *f,
                                   HYPRE_Int          *cf_marker,
                                   HYPRE_Int           relax_points,
                                   HYPRE_Real          relax_weight,
                                   HYPRE_Real          omega,
                                   HYPRE_Real         *l1_norms,
                                   hypre_ParVector    *u,
                                   hypre_ParVector    *Vtemp,
                                   hypre_ParVector    *Ztemp )
{
   const HYPRE_Int skip_diag = relax_weight == 1.0 && omega == 1.0 ? 0 : 1;

   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp,
                                                     1, 1 /* symm */, skip_diag, 0);
}


/* forward hybrid topology ordered G-S */
HYPRE_Int
hypre_BoomerAMGRelax10TopoOrderedGaussSeidel( hypre_ParCSRMatrix *A,
                                              hypre_ParVector    *f,
                                              HYPRE_Int          *cf_marker,
                                              HYPRE_Int           relax_points,
                                              HYPRE_Real          relax_weight,
                                              HYPRE_Real          omega,
                                              hypre_ParVector    *u,
                                              hypre_ParVector    *Vtemp,
                                              hypre_ParVector    *Ztemp )
{
   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, NULL, u, Vtemp, Ztemp,
                                                     1 /* forward */, 0 /* nonsymm */, 1 /* skip_diag */, 1);
}

/* forward l1 hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax13HybridL1GaussSeidel( hypre_ParCSRMatrix *A,
                                           hypre_ParVector    *f,
                                           HYPRE_Int          *cf_marker,
                                           HYPRE_Int           relax_points,
                                           HYPRE_Real          relax_weight,
                                           HYPRE_Real          omega,
                                           HYPRE_Real         *l1_norms,
                                           hypre_ParVector    *u,
                                           hypre_ParVector    *Vtemp,
                                           hypre_ParVector    *Ztemp )
{
   const HYPRE_Int skip_diag = relax_weight == 1.0 && omega == 1.0 ? 0 : 1;

   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp,
                                                     1 /* forward */, 0 /* nonsymm */, skip_diag, 0);
}

/* backward l1 hybrid G-S */
HYPRE_Int
hypre_BoomerAMGRelax14HybridL1GaussSeidel( hypre_ParCSRMatrix *A,
                                           hypre_ParVector    *f,
                                           HYPRE_Int          *cf_marker,
                                           HYPRE_Int           relax_points,
                                           HYPRE_Real          relax_weight,
                                           HYPRE_Real          omega,
                                           HYPRE_Real         *l1_norms,
                                           hypre_ParVector    *u,
                                           hypre_ParVector    *Vtemp,
                                           hypre_ParVector    *Ztemp )
{
   const HYPRE_Int skip_diag = relax_weight == 1.0 && omega == 1.0 ? 0 : 1;

   return hypre_BoomerAMGRelaxHybridGaussSeidel_core(A, f, cf_marker, relax_points, relax_weight, omega, l1_norms, u, Vtemp, Ztemp,
                                                     -1 /* backward */, 0 /* nonsymm */, skip_diag, 0);
}

HYPRE_Int
hypre_BoomerAMGRelax19GaussElim( hypre_ParCSRMatrix *A,
                                 hypre_ParVector    *f,
                                 hypre_ParVector    *u )
{
   HYPRE_BigInt     global_num_rows = hypre_ParCSRMatrixGlobalNumRows(A);
   HYPRE_BigInt     first_ind       = hypre_ParVectorFirstIndex(u);
   HYPRE_Int        n_global        = (HYPRE_Int) global_num_rows;
   HYPRE_Int        first_index     = (HYPRE_Int) first_ind;
   HYPRE_Int        num_rows        = hypre_ParCSRMatrixNumRows(A);
   hypre_Vector    *u_local         = hypre_ParVectorLocalVector(u);
   HYPRE_Complex   *u_data          = hypre_VectorData(u_local);
   hypre_CSRMatrix *A_CSR;
   HYPRE_Int       *A_CSR_i;
   HYPRE_Int       *A_CSR_j;
   HYPRE_Real      *A_CSR_data;
   hypre_Vector    *f_vector;
   HYPRE_Real      *f_vector_data;
   HYPRE_Real      *A_mat;
   HYPRE_Real      *b_vec;
   HYPRE_Int        i, jj, column, relax_error;

   /*-----------------------------------------------------------------
    *  Generate CSR matrix from ParCSRMatrix A
    *-----------------------------------------------------------------*/
#ifdef HYPRE_NO_GLOBAL_PARTITION
   /* all processors are needed for these routines */
   A_CSR = hypre_ParCSRMatrixToCSRMatrixAll(A);
   f_vector = hypre_ParVectorToVectorAll(f);
#endif
   if (num_rows)
   {
#ifndef HYPRE_NO_GLOBAL_PARTITION
      A_CSR = hypre_ParCSRMatrixToCSRMatrixAll(A);
      f_vector = hypre_ParVectorToVectorAll(f);
#endif
      A_CSR_i = hypre_CSRMatrixI(A_CSR);
      A_CSR_j = hypre_CSRMatrixJ(A_CSR);
      A_CSR_data = hypre_CSRMatrixData(A_CSR);
      f_vector_data = hypre_VectorData(f_vector);

      A_mat = hypre_CTAlloc(HYPRE_Real, n_global*n_global, HYPRE_MEMORY_HOST);
      b_vec = hypre_CTAlloc(HYPRE_Real, n_global, HYPRE_MEMORY_HOST);

      /*---------------------------------------------------------------
       *  Load CSR matrix into A_mat.
       *---------------------------------------------------------------*/
      for (i = 0; i < n_global; i++)
      {
         for (jj = A_CSR_i[i]; jj < A_CSR_i[i+1]; jj++)
         {
            column = A_CSR_j[jj];
            A_mat[i*n_global+column] = A_CSR_data[jj];
         }
         b_vec[i] = f_vector_data[i];
      }

      hypre_gselim(A_mat, b_vec, n_global, relax_error);

      for (i = 0; i < num_rows; i++)
      {
         u_data[i] = b_vec[first_index + i];
      }

      hypre_TFree(A_mat, HYPRE_MEMORY_HOST);
      hypre_TFree(b_vec, HYPRE_MEMORY_HOST);
      hypre_CSRMatrixDestroy(A_CSR);
      A_CSR = NULL;
      hypre_SeqVectorDestroy(f_vector);
      f_vector = NULL;
   }
#ifdef HYPRE_NO_GLOBAL_PARTITION
   else
   {
      hypre_CSRMatrixDestroy(A_CSR);
      A_CSR = NULL;
      hypre_SeqVectorDestroy(f_vector);
      f_vector = NULL;
   }
#endif

   return relax_error;
}

HYPRE_Int
hypre_BoomerAMGRelax98GaussElimPivot( hypre_ParCSRMatrix *A,
                                      hypre_ParVector    *f,
                                      hypre_ParVector    *u )
{
   HYPRE_BigInt     global_num_rows = hypre_ParCSRMatrixGlobalNumRows(A);
   HYPRE_BigInt     first_ind       = hypre_ParVectorFirstIndex(u);
   HYPRE_Int        n_global        = (HYPRE_Int) global_num_rows;
   HYPRE_Int        first_index     = (HYPRE_Int) first_ind;
   HYPRE_Int        num_rows        = hypre_ParCSRMatrixNumRows(A);
   hypre_Vector    *u_local         = hypre_ParVectorLocalVector(u);
   HYPRE_Complex   *u_data          = hypre_VectorData(u_local);
   hypre_CSRMatrix *A_CSR;
   HYPRE_Int       *A_CSR_i;
   HYPRE_Int       *A_CSR_j;
   HYPRE_Real      *A_CSR_data;
   hypre_Vector    *f_vector;
   HYPRE_Real      *f_vector_data;
   HYPRE_Real      *A_mat;
   HYPRE_Real      *b_vec;
   HYPRE_Int        i, jj, column, relax_error = 0;
   HYPRE_Int        info;
   HYPRE_Int        one_i = 1;
   HYPRE_Int       *piv;

   /*-----------------------------------------------------------------
    *  Generate CSR matrix from ParCSRMatrix A
    *-----------------------------------------------------------------*/
#ifdef HYPRE_NO_GLOBAL_PARTITION
   /* all processors are needed for these routines */
   A_CSR = hypre_ParCSRMatrixToCSRMatrixAll(A);
   f_vector = hypre_ParVectorToVectorAll(f);
#endif
   if (num_rows)
   {
#ifndef HYPRE_NO_GLOBAL_PARTITION
      A_CSR = hypre_ParCSRMatrixToCSRMatrixAll(A);
      f_vector = hypre_ParVectorToVectorAll(f);
#endif
      A_CSR_i = hypre_CSRMatrixI(A_CSR);
      A_CSR_j = hypre_CSRMatrixJ(A_CSR);
      A_CSR_data = hypre_CSRMatrixData(A_CSR);
      f_vector_data = hypre_VectorData(f_vector);

      A_mat = hypre_CTAlloc(HYPRE_Real,  n_global*n_global, HYPRE_MEMORY_HOST);
      b_vec = hypre_CTAlloc(HYPRE_Real,  n_global, HYPRE_MEMORY_HOST);

      /*---------------------------------------------------------------
       *  Load CSR matrix into A_mat.
       *---------------------------------------------------------------*/
      for (i = 0; i < n_global; i++)
      {
         for (jj = A_CSR_i[i]; jj < A_CSR_i[i+1]; jj++)
         {
            /* need col major */
            column = A_CSR_j[jj];
            A_mat[i + n_global*column] = A_CSR_data[jj];
         }
         b_vec[i] = f_vector_data[i];
      }

      piv = hypre_CTAlloc(HYPRE_Int,  n_global, HYPRE_MEMORY_HOST);

      /* write over A with LU */
      hypre_dgetrf(&n_global, &n_global, A_mat, &n_global, piv, &info);

      /*now b_vec = inv(A)*b_vec  */
      hypre_dgetrs("N", &n_global, &one_i, A_mat, &n_global, piv, b_vec, &n_global, &info);

      hypre_TFree(piv, HYPRE_MEMORY_HOST);

      for (i = 0; i < num_rows; i++)
      {
         u_data[i] = b_vec[first_index+i];
      }

      hypre_TFree(A_mat, HYPRE_MEMORY_HOST);
      hypre_TFree(b_vec, HYPRE_MEMORY_HOST);
      hypre_CSRMatrixDestroy(A_CSR);
      A_CSR = NULL;
      hypre_SeqVectorDestroy(f_vector);
      f_vector = NULL;
   }
#ifdef HYPRE_NO_GLOBAL_PARTITION
   else
   {
      hypre_CSRMatrixDestroy(A_CSR);
      A_CSR = NULL;
      hypre_SeqVectorDestroy(f_vector);
      f_vector = NULL;
   }
#endif

   return relax_error;
}

HYPRE_Int
hypre_BoomerAMGRelaxKaczmarz( hypre_ParCSRMatrix *A,
                              hypre_ParVector    *f,
                              HYPRE_Real          omega,
                              HYPRE_Real         *l1_norms,
                              hypre_ParVector    *u )
{
   MPI_Comm             comm          = hypre_ParCSRMatrixComm(A);
   hypre_CSRMatrix     *A_diag        = hypre_ParCSRMatrixDiag(A);
   HYPRE_Real          *A_diag_data   = hypre_CSRMatrixData(A_diag);
   HYPRE_Int           *A_diag_i      = hypre_CSRMatrixI(A_diag);
   HYPRE_Int           *A_diag_j      = hypre_CSRMatrixJ(A_diag);
   hypre_CSRMatrix     *A_offd        = hypre_ParCSRMatrixOffd(A);
   HYPRE_Int           *A_offd_i      = hypre_CSRMatrixI(A_offd);
   HYPRE_Real          *A_offd_data   = hypre_CSRMatrixData(A_offd);
   HYPRE_Int           *A_offd_j      = hypre_CSRMatrixJ(A_offd);
   hypre_ParCSRCommPkg *comm_pkg      = hypre_ParCSRMatrixCommPkg(A);
   HYPRE_Int            num_rows      = hypre_CSRMatrixNumRows(A_diag);
   HYPRE_Int            num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   hypre_Vector        *u_local       = hypre_ParVectorLocalVector(u);
   HYPRE_Complex       *u_data        = hypre_VectorData(u_local);
   hypre_Vector        *f_local       = hypre_ParVectorLocalVector(f);
   HYPRE_Complex       *f_data        = hypre_VectorData(f_local);
   HYPRE_Complex       *u_offd_data   = NULL;
   HYPRE_Complex       *u_buf_data    = NULL;
   HYPRE_Complex        res;

   HYPRE_Int num_procs, my_id, i, j, index, num_sends, start;
   hypre_ParCSRCommHandle *comm_handle;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   if (num_procs > 1)
   {
      if (!comm_pkg)
      {
         hypre_MatvecCommPkgCreate(A);
         comm_pkg = hypre_ParCSRMatrixCommPkg(A);
      }

      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      u_buf_data = hypre_TAlloc(HYPRE_Real, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends), HYPRE_MEMORY_HOST);
      u_offd_data = hypre_TAlloc(HYPRE_Real, num_cols_offd, HYPRE_MEMORY_HOST);

      index = 0;
      for (i = 0; i < num_sends; i++)
      {
         start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
         for (j = start; j < hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1); j++)
         {
            u_buf_data[index++] = u_data[hypre_ParCSRCommPkgSendMapElmt(comm_pkg, j)];
         }
      }

      comm_handle = hypre_ParCSRCommHandleCreate(1, comm_pkg, u_buf_data, u_offd_data);
      hypre_ParCSRCommHandleDestroy(comm_handle);
      hypre_TFree(u_buf_data, HYPRE_MEMORY_HOST);
   }

   /* Forward local pass */
   for (i = 0; i < num_rows; i++)
   {
      res = f_data[i];
      for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
      {
         res -= A_diag_data[j] * u_data[A_diag_j[j]];
      }

      for (j = A_offd_i[i]; j < A_offd_i[i+1]; j++)
      {
         res -= A_offd_data[j] * u_offd_data[A_offd_j[j]];
      }

      res /= l1_norms[i];

      for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
      {
         u_data[A_diag_j[j]] += omega * res * A_diag_data[j];
      }
   }

   /* Backward local pass */
   for (i = num_rows - 1; i > -1; i--)
   {
      res = f_data[i];
      for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
      {
         res -= A_diag_data[j] * u_data[A_diag_j[j]];
      }

      for (j = A_offd_i[i]; j < A_offd_i[i+1]; j++)
      {
         res -= A_offd_data[j] * u_offd_data[A_offd_j[j]];
      }

      res /= l1_norms[i];

      for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
      {
         u_data[A_diag_j[j]] += omega * res * A_diag_data[j];
      }
   }

   hypre_TFree(u_offd_data, HYPRE_MEMORY_HOST);

   return hypre_error_flag;
}

