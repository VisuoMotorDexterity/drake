#include "drake/examples/pendulum/pendulum_plant.h"

#include "drake/common/drake_throw.h"
#include "drake/common/eigen_autodiff_types.h"

namespace drake {
namespace examples {
namespace pendulum {

template <typename T>
PendulumPlant<T>::PendulumPlant()
    : systems::LeafSystem<T>(
          systems::SystemTypeTag<pendulum::PendulumPlant>{}) {
  this->DeclareInputPort(systems::kVectorValued, 1);
  this->DeclareVectorOutputPort(PendulumStateVector<T>(),
                                &PendulumPlant::CopyStateOut);
  this->DeclareContinuousState(
      PendulumStateVector<T>(),
      1 /* num_q */, 1 /* num_v */, 0 /* num_z */);
  static_assert(PendulumStateVectorIndices::kNumCoordinates == 1 + 1, "");
}

template <typename T>
template <typename U>
PendulumPlant<T>::PendulumPlant(const PendulumPlant<U>&)
    : PendulumPlant() {}

template <typename T>
PendulumPlant<T>::~PendulumPlant() {}

template <typename T>
const systems::InputPortDescriptor<T>&
PendulumPlant<T>::get_tau_port() const {
  return this->get_input_port(0);
}

template <typename T>
const systems::OutputPort<T>&
PendulumPlant<T>::get_output_port() const {
  return systems::System<T>::get_output_port(0);
}

template <typename T>
void PendulumPlant<T>::CopyStateOut(const systems::Context<T>& context,
                                    PendulumStateVector<T>* output) const {
  output->set_value(get_state(context).get_value());
}

template <typename T>
T PendulumPlant<T>::CalcTotalEnergy(const systems::Context<T>& context) const {
  using std::pow;
  const PendulumStateVector<T>& state = get_state(context);
  // Kinetic energy = 1/2 m l² θ̇².
  const T kinetic_energy = 0.5 * m() * pow(l() * state.thetadot(), 2);
  // Potential energy = -mgl cos θ.
  const T potential_energy = -m() * g() * l() * cos(state.theta());
  return kinetic_energy + potential_energy;
}

// Compute the actual physics.
template <typename T>
void PendulumPlant<T>::DoCalcTimeDerivatives(
    const systems::Context<T>& context,
    systems::ContinuousState<T>* derivatives) const {
  const PendulumStateVector<T>& state = get_state(context);
  PendulumStateVector<T>* derivative_vector = get_mutable_state(derivatives);

  derivative_vector->set_theta(state.thetadot());
  // Pendulum formula from Section 2.2 of Russ Tedrake. Underactuated
  // Robotics: Algorithms for Walking, Running, Swimming, Flying, and
  // Manipulation (Course Notes for MIT 6.832). Downloaded on
  // 2016-09-30 from
  // http://underactuated.csail.mit.edu/underactuated.html?chapter=2
  derivative_vector->set_thetadot(
      (get_tau(context) - m_ * g_ * lc_ * sin(state.theta()) -
       b_ * state.thetadot()) / I_);
}

template class PendulumPlant<double>;
template class PendulumPlant<AutoDiffXd>;
template class PendulumPlant<symbolic::Expression>;

}  // namespace pendulum
}  // namespace examples
}  // namespace drake
