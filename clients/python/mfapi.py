"""
This module declares the API for the monitoring framework.
"""
from json import dumps as to_json
from json import loads
from logging import basicConfig, debug, getLogger, info, DEBUG, WARNING
from requests import get, post, put
from sys import exit, stderr

class API(object):
  """Interface to provide access to the monitoring database.

    Args:
      url: The base URL to access the monitoring framework (optional)
      debug: The debug level as imposed by the logging module (optional)

    Attributes:
      url: The base URL to access the monitoring framework (optional)
      debug: The debug level as imposed by the logging module (optional)
  """

  """ Shared variables """
  experiments = 'experiments'
  deployments = 'deployments'
  metrics = 'metrics'
  profiles = 'profiles'
  workflows = 'workflows'
  runtime = 'runtime'
  progress = 'progress'
  statistics = 'statistics'

  def __init__(self, url=None, debug=None):
    """Initializes the API.

      Sets the appropriate base URL to access the monitoring framework, and sets
      the corresponding debug level. The base URL is appended by the service
      prefix for subsequent requests.

      Args:
        url: The base URL to access the monitoring framework (optional)
        agent: A layer to store information on where the data is coming from
        host: Unique identifier of the device the agent job was completed
        platform: Type of host (e.g., VM server, FPGA1, PI, ...)
        debug: The debug level as imposed by the logging module (optional)
    """
    if url is None:
      url = 'http://mf.excess-project.eu:3030'
    if debug is not None:
      basicConfig(stream=stderr, level=debug)
    self.url = self.urljoin(url, 'v1/dreamcloud/mf')
    self.check_url(self.url)
    self.agent = None
    self.host = None
    self.platform = None

  def check_url(self, url):
    """Checks if the given URL is valid.

      Performs a simple GET request on the given URL; expects status code 200.

      Args:
        url: The base URL to access the monitoring framework
    """
    debug('check url ...')
    info('[ url: ' + url + ' ]')
    try:
      request = get(url)
    except:
      exit('[ error: ' + url + ' not accessible]')

  @staticmethod
  def urljoin(*args):
    return "/".join(map(lambda x: str(x).rstrip('/'), args))

  def set_host(self, host_id):
    self.host = host_id

  def set_platform(self, platform):
    self.platform = platform


class ProfilingAPI(API):
  """API for the DreamCloud Monitoring Framework.

    This class implements the protocol to access the monitoring framework using
    the underlying RESTful web service provided. For more information on the
    API, please have a look at the official API specification.

    Note:
      This API is used to send metric data to the database.

    Attributes:
      url: The base URL to access the monitoring framework (optional)
      debug: The debug level as imposed by the logging module (optional)
  """

  def __init__(self, url=None, debug=None):
    API.__init__(self, url, debug)
    self.headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}

  def new_experiment(self, workflow_id, data):
    """Creates a new experiment for the given workflow ID.

      Registers a new workflow at the monitoring database by using a unique
      workflow ID and the given JSON object. If successful, a new experiment
      is created to allow sending metric data. Please note that if the
      workflow ID already exists in the database, the data will be overridden
      with new data provided. The JSON object is neither parsed or checked,
      and will be inserted into the database as is.

      Args:
        workflow_id: A unique ID representing the workflow to be registered (e.g., 'ms2_v2')
        data: JSON object to be stored at the monitoring database (e.g., { 'application': 'molecular dynamics' })

      Returns:
        an experiment ID
      Raises:
        Exception if an error occurred while contacting the database
    """
    debug('register workflow and create experiment ...')
    workflow_id = workflow_id.lower()
    data['wf_id'] = workflow_id
    workflow_url = self.urljoin(self.url, self.workflows)
    request = put(workflow_url, data=to_json(data), headers=self.headers)
    if request.status_code == 200:
      info('[ workflow_id: ' + workflow_id + ' ]')
      experiment_id = request.json()['experiment']['id']
      info('[ experiment_id: ' +  experiment_id + ' ]')
      return experiment_id
    else:
      raise Exception(request.json())

  def add_deployment_plan(self, workflow_id, task_id, platform, experiment_id, data):
    """Adds new deployment plan information.

      Registers a new deployment plan on the monitoring database. Deployment
      plans are linked to the current workflow ID, task ID, platform, and
      experiment ID.

      Args:
        workflow_id: A unique ID representing the workflow to be registered (e.g., 'ms2_v2')
        task_id: A unique ID representing a specific task within the workflow (e.g., 'task_1')
        platform: Target platform where the task will be executed (e.g., 'hpc_cluster')
        experiment_id: A unique ID as returned by the function 'new_experiment'
        data: JSON object to be stored at the monitoring database (the actual deployment plan)
      Returns:
        a deployment plan ID
      Raises:
        Exception if an error occurred while contacting the database
    """
    debug('add new deployment plan ...')
    workflow_id = workflow_id.lower()
    task_id = task_id.lower()
    platform = platform.lower()

    deployment_url = self.urljoin(self.url,
            self.deployments, workflow_id, task_id, platform, experiment_id)

    print deployment_url

    request = put(deployment_url, data=to_json(data), headers=self.headers)
    if request.status_code == 200:
      info('[ deployment: ' + request.json()['href'] + ' ]')
    else:
      raise Exception(request.json())

  def update(self, workflow_id, experiment_id, data, task_id=None):
    """Sends metric data to the monitoring database.

      Sends the given metric data (JSON object) to the monitoring database. In
      order to associate the data with the corresponding workflow, the workflow
      ID, experiment ID, and the task ID (only required if the application is
      workflow-based) have to be specified as well.

      Args:
        workflow_id: A unique ID representing the workflow to be registered (e.g., 'ms2_v2')
        experiment_id: A unique ID as returned by the function 'new_experiment'
        data: JSON object to be stored at the monitoring database
        task_id: A unique ID representing a specific task within the workflow (e.g., 'task_1')
      Raises:
        Exception if an error occurred while contacting the database
    """
    debug('send metric data ...')
    workflow_id = workflow_id.lower()
    metrics_url = self.urljoin(self.url, self.metrics, workflow_id, experiment_id)
    if task_id is None:
      task_id = "_all";
    if task_id is not None:
      task_id = task_id.lower()
      metrics_url = metrics_url + '?task=' + task_id

    self.extend_data(data, task_id)

    request = post(metrics_url, data=to_json(data), headers=self.headers)
    if request.status_code == 200:
      metric_id = request.json().iterkeys().next()
      info('[ profile: ' + request.json()[metric_id]['href'] + ' ]')
    else:
      raise Exception(request.json())

  def bulk_update(self, bulk_data):
    """Sends metric data to the monitoring database; bulk method.

       Please refer to the update method for further information.

       Args:
         bulk_data: array of metric data for the same experiment and workflow

       Raises:
         Exception if an error occurred while contacting the database
    """
    debug('send metric data as a bulk query...')
    metrics_url = self.urljoin(self.url, self.metrics)

    for data in bulk_data:
      self.extend_data(data)

    request = post(metrics_url, data=to_json(bulk_data), headers=self.headers)
    if request.status_code == 200:
      for element in request.json():
        info('[ profile: ' + element + ' ]')
    else:
      raise Exception(request.json())


  def extend_data(self, data, task_id):
    if self.host is not None:
      data['host'] = self.host
    if self.platform is not None:
      data['platform'] = self.platform


class ExploringAPI(API):
  """API for the DreamCloud Monitoring Framework.

    This class implements the protocol to access the monitoring framework using
    the underlying RESTful web service provided. For more information on the
    API, please have a look at the official API specification.

    Note:
      This API is used to retrieve metric data from the database.

    Attributes:
      url: The base URL to access the monitoring framework (optional)
      debug: The debug level as imposed by the logging module (optional)
  """

  def __init__(self, url=None, debug=None):
    API.__init__(self, url, debug)

  def handle_request(self, url):
    """Handles requests.

      Note:
        This method is only used internally. If a requests failed, the program
        exits with the corresponding error message.

      Args:
        url: The URL to be accessed.
    """
    request = get(url)
    if request.status_code == 200:
      return request.json()
    else:
      exit(request.json())

  def get_workflows(self, details=None):
    """GET /mf/workflows."""

    debug('get all registered workflows ...')
    workflows_url = self.urljoin(self.url, self.workflows)
    if details is not None:
      workflows_url += '?details'
    return self.handle_request(workflows_url)

  def get_workflow(self, workflow_id):
    """GET /mf/workflows/:workID."""

    debug('get all experiments for workflow "' + workflow_id + '" ...')
    workflow_url = self.urljoin(self.url, self.workflows, workflow_id)
    return self.handle_request(workflow_url)

  def get_profiles(self, workflow_id, task_id= None):
    """GET /mf/profiles/:workID/:taskID."""

    debug('get all available profiles for "' + workflow_id + '" ...')
    profile_url = self.urljoin(self.url, self.profiles, workflow_id)
    if task_id is not None:
      profile_url += '/' + task_id
    return self.handle_request(profile_url)

  def get_progress(self, workflow_id, task_id, experiment_id):
    """GET /mf/progress/:workID/:taskID/:expID."""

    debug('get progress for experiment "' + experiment_id + '" ...')
    progress_url = self.urljoin(self.url, self.progress, workflow_id, task_id, experiment_id)
    return self.handle_request(progress_url)

  def get_profile(self, workflow_id, task_id, experiment_id):
    """GET /mf/profiles/:workID/:taskID/:expID."""

    debug('get profile for experiment "' + experiment_id + '" ...')
    profile_url = self.urljoin(self.url, self.profiles, workflow_id, task_id, experiment_id)
    return self.handle_request(profile_url)

  def get_runtime(self, workflow_id, task_id, experiment_id):
    """GET /mf/runtime/:workID/:taskID/:expID."""

    debug('get runtime for experiment "' + experiment_id + '" ...')
    runtime_url = self.urljoin(self.url, self.runtime, workflow_id, task_id, experiment_id)
    return self.handle_request(runtime_url)

  def get_total_runtime(self, workflow_id, experiment_id):
    """GET /mf/runtime/:workID/:expID."""

    debug('get runtime for experiment "' + experiment_id + '" ...')
    runtime_url = self.urljoin(self.url, self.runtime, workflow_id, experiment_id)
    return self.handle_request(runtime_url)

  def get_statistics(self, workflow_id, metric):
    """GET /mf/statistics/:workID?metric=metric"""

    debug('get statistics for workflow "' + workflow_id + '" and metric "' + metric + '"')
    statistics_url = self.urljoin(self.url, self.statistics, workflow_id)
    statistics_url = statistics_url + '?metric=' + metric
    return self.handle_request(statistics_url)

getLogger('urllib3').setLevel(WARNING)
