{
    rho = thermo.rho();

    // Thermodynamic density needs to be updated by psi*d(p) after the
    // pressure solution - done in 2 parts. Part 1:
    thermo.rho() -= psi*p;

    volScalarField rUA = 1.0/UEqn.A();
    U = rUA*UEqn.H();

    if (pimple.transonic())
    {
        surfaceScalarField phiv =
            (fvc::interpolate(U) & mesh.Sf())
          + fvc::ddtPhiCorr(rUA, rho, U, phi);

        phi = fvc::interpolate(rho)*phiv;

        surfaceScalarField phid
        (
            "phid",
            fvc::interpolate(thermo.psi())*phiv
        );

        while (pimple.correctNonOrthogonal())
        {
            fvScalarMatrix pEqn
            (
                fvc::ddt(rho) + fvc::div(phi)
              + correction(fvm::ddt(psi, p) + fvm::div(phid, p))
              - fvm::laplacian(rho*rUA, p)
            );

            pEqn.solve
            (
                mesh.solutionDict().solver(p.select(pimple.finalInnerIter()))
            );

            if (pimple.finalNonOrthogonalIter())
            {
                phi += pEqn.flux();
            }
        }
    }
    else
    {
        phi =
            fvc::interpolate(rho)
           *(
                (fvc::interpolate(U) & mesh.Sf())
              + fvc::ddtPhiCorr(rUA, rho, U, phi)
            );

        while (pimple.correctNonOrthogonal())
        {
            fvScalarMatrix pEqn
            (
                fvc::ddt(rho) + psi*correction(fvm::ddt(p))
              + fvc::div(phi)
              - fvm::laplacian(rho*rUA, p)
            );

            pEqn.solve
            (
                mesh.solutionDict().solver(p.select(pimple.finalInnerIter()))
            );

            if (pimple.finalNonOrthogonalIter())
            {
                phi += pEqn.flux();
            }
        }
    }

    // Second part of thermodynamic density update
    thermo.rho() += psi*p;

    #include "rhoEqn.H"
    #include "compressibleContinuityErrs.H"

    U -= rUA*fvc::grad(p);
    U.correctBoundaryConditions();

    DpDt = fvc::DDt(surfaceScalarField("phiU", phi/fvc::interpolate(rho)), p);
}
